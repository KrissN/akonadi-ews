/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewssyncfolderhierarchyrequest.h"

#include <memory>

#include <QXmlStreamWriter>

#include "ewsclient_debug.h"
#include "ewsxml.h"

enum SyncFolderHierarchyResponseElementType {
    SyncState,
    IncludesLastFolderInRange,
    Changes
};

enum SyncFolderHierarchyChangeElementType {
    Folder,
    FolderId,
    IsRead
};

class EwsSyncFolderHierarchyRequest::Response : public EwsRequest::Response
{
public:
    Response(QXmlStreamReader &reader);

    static bool changeReader(QXmlStreamReader &reader, QVariant &val);

    EwsSyncFolderHierarchyRequest::Change::List mChanges;
    bool mIncludesLastFolder;
    QString mSyncState;
};

EwsSyncFolderHierarchyRequest::EwsSyncFolderHierarchyRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent), mIncludesLastItem(false)
{
    qRegisterMetaType<EwsSyncFolderHierarchyRequest::Change::List>();
    qRegisterMetaType<EwsFolder>();
}

EwsSyncFolderHierarchyRequest::~EwsSyncFolderHierarchyRequest()
{
}

void EwsSyncFolderHierarchyRequest::setFolderId(const EwsId &id)
{
    mFolderId = id;
}

void EwsSyncFolderHierarchyRequest::setFolderShape(const EwsFolderShape &shape)
{
    mShape = shape;
}

void EwsSyncFolderHierarchyRequest::setSyncState(const QString &state)
{
    mSyncState = state;
}

void EwsSyncFolderHierarchyRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderHierarchy"));

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderId"));
    mFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    if (!mSyncState.isNull()) {
        writer.writeTextElement(ewsMsgNsUri, QStringLiteral("SyncState"), mSyncState);
    }

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        QString st = mSyncState.isNull() ? QStringLiteral("none") : QString::number(qHash(mSyncState), 36);
        QString folder;
        qCDebugNCS(EWSRES_REQUEST_LOG) << QStringLiteral("Starting SyncFolderHierarchy request (folder: ")
                                       << mFolderId << QStringLiteral(", state: %1").arg(st);
    }

    prepare(reqString);

    doSend();
}

bool EwsSyncFolderHierarchyRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("SyncFolderHierarchy"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsSyncFolderHierarchyRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    EwsSyncFolderHierarchyRequest::Response *resp = new EwsSyncFolderHierarchyRequest::Response(reader);
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mChanges = resp->mChanges;
    mSyncState = resp->mSyncState;
    mIncludesLastItem = resp->mIncludesLastFolder;

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp->isSuccess()) {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got SyncFolderHierarchy response (%1 changes, state: %3)")
                                          .arg(mChanges.size()).arg(qHash(mSyncState), 0, 36);
        } else {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got SyncFolderHierarchy response - %1")
                                          .arg(resp->responseMessage());
        }
    }

    return true;
}

EwsSyncFolderHierarchyRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    static const QVector<EwsXml<SyncFolderHierarchyResponseElementType>::Item> items = {
        {SyncState, QStringLiteral("SyncState"), &ewsXmlTextReader},
        {IncludesLastFolderInRange, QStringLiteral("IncludesLastFolderInRange"), &ewsXmlBoolReader},
        {Changes, QStringLiteral("Changes"), &EwsSyncFolderHierarchyRequest::Response::changeReader},
    };
    static const EwsXml<SyncFolderHierarchyResponseElementType> staticReader(items);

    EwsXml<SyncFolderHierarchyResponseElementType> ewsReader(staticReader);

    if (!ewsReader.readItems(reader, ewsMsgNsUri,
        [this](QXmlStreamReader &reader, const QString &) {
            if (!readResponseElement(reader)) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
                return false;
            }
            return true;
        }))
    {
        mClass = EwsResponseParseError;
        return;
    }

    QHash<SyncFolderHierarchyResponseElementType, QVariant> values = ewsReader.values();

    mSyncState = values[SyncState].toString();
    mIncludesLastFolder = values[IncludesLastFolderInRange].toBool();
    mChanges = values[Changes].value<Change::List>();
}

bool EwsSyncFolderHierarchyRequest::Response::changeReader(QXmlStreamReader &reader, QVariant &val)
{
    Change::List changes;
    QString elmName(reader.name().toString());

    while (reader.readNextStartElement()) {
        Change change(reader);
        if (!change.isValid()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element").arg(elmName);
            return false;
        }
        changes.append(change);
    }

    val = QVariant::fromValue<Change::List>(changes);
    return true;
}

EwsSyncFolderHierarchyRequest::Change::Change(QXmlStreamReader &reader)
{
    static const QVector<EwsXml<SyncFolderHierarchyChangeElementType>::Item> items = {
        {Folder, QStringLiteral("Folder"), &ewsXmlFolderReader},
        {Folder, QStringLiteral("CalendarFolder"), &ewsXmlFolderReader},
        {Folder, QStringLiteral("ContactsFolder"), &ewsXmlFolderReader},
        {Folder, QStringLiteral("SearchFolder"), &ewsXmlFolderReader},
        {Folder, QStringLiteral("TasksFolder"), &ewsXmlFolderReader},
        {FolderId, QStringLiteral("FolderId"), &ewsXmlIdReader},
        {IsRead, QStringLiteral("IsRead"), &ewsXmlBoolReader}
    };
    static const EwsXml<SyncFolderHierarchyChangeElementType> staticReader(items);

    EwsXml<SyncFolderHierarchyChangeElementType> ewsReader(staticReader);

    if (reader.name() == QStringLiteral("Create")) {
        mType = Create;
    } else if (reader.name() == QStringLiteral("Update")) {
        mType = Update;
    } else if (reader.name() == QStringLiteral("Delete")) {
        mType = Delete;
    }
    if (!ewsReader.readItems(reader, ewsTypeNsUri)) {
        return;
    }

    QHash<SyncFolderHierarchyChangeElementType, QVariant> values = ewsReader.values();

    switch (mType) {
    case Create:
    case Update:
        mFolder = values[Folder].value<EwsFolder>();
        mId = mFolder[EwsFolderFieldFolderId].value<EwsId>();
        break;
    case Delete:
        mId = values[FolderId].value<EwsId>();
        break;
    default:
        break;
    }
}
