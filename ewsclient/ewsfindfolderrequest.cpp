/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <memory>
#include <QtCore/QXmlStreamWriter>

#include "ewsfindfolderrequest.h"
#include "ewsclient_debug.h"

static const QString traversalTypeNames[] = {
    QStringLiteral("Shallow"),
    QStringLiteral("Deep"),
    QStringLiteral("SoftDeleted")
};

EwsFindFolderRequest::EwsFindFolderRequest(EwsClient& client, QObject *parent)
    : EwsRequest(client, parent), mTraversal(EwsTraversalDeep)
{
}

EwsFindFolderRequest::~EwsFindFolderRequest()
{
}

void EwsFindFolderRequest::setParentFolderId(const EwsId &id)
{
    mParentId = id;
}

void EwsFindFolderRequest::setFolderShape(const EwsFolderShape &shape)
{
    mShape = shape;
}

void EwsFindFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FindFolder"));
    writer.writeAttribute(QStringLiteral("Traversal"), traversalTypeNames[mTraversal]);

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ParentFolderIds"));
    mParentId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLIENT_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsFindFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("FindFolder"),
                                [this](QXmlStreamReader &reader){return parseFoldersResponse(reader);});
}

bool EwsFindFolderRequest::parseFoldersResponse(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri || reader.name() != QStringLiteral("RootFolder"))
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element (got %2).")
                        .arg(QStringLiteral("RootFolder")).arg(reader.qualifiedName().toString()));

    if (!reader.attributes().hasAttribute(QStringLiteral("TotalItemsInView"))
        || !reader.attributes().hasAttribute(QStringLiteral("TotalItemsInView"))) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - missing attributes of %1 element.")
                                .arg(QStringLiteral("RootFolder")));
    }
    bool ok;
    unsigned totalItems = reader.attributes().value(QStringLiteral("TotalItemsInView")).toUInt(&ok);
    if (!ok)
        return setErrorMsg(QStringLiteral("Failed to read EWS request - failed to read %1 attribute.")
                                        .arg(QStringLiteral("TotalItemsInView")));

    if (!reader.readNextStartElement())
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.")
                        .arg(QStringLiteral("RootFolder")));

    if (reader.namespaceUri() != ewsTypeNsUri || reader.name() != QStringLiteral("Folders"))
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element (got %2).")
                        .arg(QStringLiteral("Folders")).arg(reader.qualifiedName().toString()));

    if (!reader.readNextStartElement())
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.")
                        .arg(QStringLiteral("Folders")));

    if (reader.namespaceUri() != ewsTypeNsUri)
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected child element from types namespace."));

    unsigned i = 0;
    for (i = 0; i < totalItems; i++) {
        EwsFolder *folder = readFolder(reader);
        reader.readNextStartElement();
        if (folder) {
            bool ok;
            int childCount = (*folder)[EwsFolderFieldChildFolderCount].toUInt(&ok);
            qCDebug(EWSCLIENT_LOG).noquote() << QStringLiteral("Folder %1 has %2 children")
                            .arg((*folder)[EwsFolderFieldDisplayName].toString()).arg(childCount);
            if (childCount > 0) {
                unsigned readCount = readChildFolders(*folder, childCount, reader);
                if (readCount == 0)
                    return false;
                i += readCount;
            }
            mFolders.append(*folder);
        }
    }

    return true;
}

unsigned EwsFindFolderRequest::readChildFolders(EwsFolder &parent, unsigned count, QXmlStreamReader &reader)
{
    qCDebug(EWSCLIENT_LOG).noquote() << QStringLiteral("Processing %1 folder").arg(parent[EwsFolderFieldDisplayName].toString());
    unsigned readCount = 0;
    for (unsigned i = 0; i < count; i++) {
        EwsFolder *folder = readFolder(reader);
        reader.readNextStartElement();
        if (folder) {
            bool ok;
            int childCount = (*folder)[EwsFolderFieldChildFolderCount].toUInt(&ok);
            if (ok && childCount > 0) {
                unsigned readCount2 = readChildFolders(*folder, childCount, reader);
                if (readCount2 == 0)
                    return false;
                readCount += readCount2;
            }
            parent.addChild(*folder);
        }
        readCount++;
    }
    return readCount;
}

EwsFolder* EwsFindFolderRequest::readFolder(QXmlStreamReader &reader)
{
    EwsFolder *folder = 0;
    if (reader.name() == QStringLiteral("Folder") ||
        reader.name() == QStringLiteral("CalendarFolder") ||
        reader.name() == QStringLiteral("ContactsFolder") ||
        reader.name() == QStringLiteral("TasksFolder") ||
        reader.name() == QStringLiteral("SearchFolder")) {
        qCDebug(EWSCLIENT_LOG).noquote() << QStringLiteral("Processing folder");
        folder = new EwsFolder(reader);
        if (!folder->isValid()) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid %1 element.")
                     .arg(QStringLiteral("Folder")));
            return 0;
        }
    }
    else {
        qCWarning(EWSCLIENT_LOG).noquote() << QStringLiteral("Unsupported folder type %1").arg(reader.name().toString());
        reader.skipCurrentElement();
    }

    return folder;
}

