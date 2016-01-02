/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015-2016 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewsfinditemrequest.h"
#include "ewsclient_debug.h"

static const QString traversalTypeNames[] = {
    QStringLiteral("Shallow"),
    QStringLiteral("Deep"),
    QStringLiteral("SoftDeleted"),
    QStringLiteral("Associated")
};

EwsFindItemRequest::EwsFindItemRequest(EwsClient& client, QObject *parent)
    : EwsRequest(client, parent), mTraversal(EwsTraversalShallow), mPageSize(0), mPage(0)
{
}

EwsFindItemRequest::~EwsFindItemRequest()
{
}

void EwsFindItemRequest::setFolderId(const EwsId &id)
{
    mFolderId = id;
}

void EwsFindItemRequest::setItemShape(const EwsItemShape &shape)
{
    mShape = shape;
}

void EwsFindItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FindItem"));
    writer.writeAttribute(QStringLiteral("Traversal"), traversalTypeNames[mTraversal]);

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ParentFolderIds"));
    mFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLIENT_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsFindItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("FindItem"),
                                [this](QXmlStreamReader &reader){return parseItemsResponse(reader);});
}

bool EwsFindItemRequest::parseItemsResponse(QXmlStreamReader &reader)
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

    if (reader.namespaceUri() != ewsTypeNsUri || reader.name() != QStringLiteral("Items"))
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element (got %2).")
                        .arg(QStringLiteral("Items")).arg(reader.qualifiedName().toString()));

    if (!reader.readNextStartElement())
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.")
                        .arg(QStringLiteral("Items")));

    if (reader.namespaceUri() != ewsTypeNsUri)
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected child element from types namespace."));

    unsigned i = 0;
    for (i = 0; i < totalItems; i++) {
        EwsItem*item = readItem(reader);
        reader.readNextStartElement();
        if (item) {
            mItems.append(*item);
        }
    }

    return true;
}

EwsItem* EwsFindItemRequest::readItem(QXmlStreamReader &reader)
{
    EwsItem *item = 0;
    if (reader.name() == QStringLiteral("Item") ||
        reader.name() == QStringLiteral("Message") ||
        reader.name() == QStringLiteral("CalendarItem") ||
        reader.name() == QStringLiteral("Contact") ||
        reader.name() == QStringLiteral("DistributionList") ||
        reader.name() == QStringLiteral("MeetingMessage") ||
        reader.name() == QStringLiteral("MeetingRequest") ||
        reader.name() == QStringLiteral("MeetingResponse") ||
        reader.name() == QStringLiteral("MeetingCancellation") ||
        reader.name() == QStringLiteral("Task")) {
        qCDebug(EWSCLIENT_LOG).noquote() << QStringLiteral("Processing %1").arg(reader.name().toString());
        item = new EwsItem(reader);
        if (!item->isValid()) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid %1 element.")
                     .arg(reader.name().toString()));
            return 0;
        }
    }
    else {
        qCWarning(EWSCLIENT_LOG).noquote() << QStringLiteral("Unsupported folder type %1").arg(reader.name().toString());
        reader.skipCurrentElement();
    }

    return item;
}

