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

#include "ewssyncfolderitemsrequest.h"
#include "ewsxmlreader.h"
#include "ewsclient_debug.h"

enum SyncResponseElementType {
    SyncState,
    IncludesLastItemInRange,
    Changes
};

enum ChangeElementType {
    Item,
    ItemId,
    IsRead
};

class EwsSyncFolderItemsRequest::Response : public EwsRequest::Response
{
public:
    Response(QXmlStreamReader &reader);

    static bool changeReader(QXmlStreamReader &reader, QVariant &val);

    EwsSyncFolderItemsRequest::Change::List mChanges;
    bool mIncludesLastItem;
    QString mSyncState;
};

EwsSyncFolderItemsRequest::EwsSyncFolderItemsRequest(EwsClient& client, QObject *parent)
    : EwsRequest(client, parent), mMaxChanges(100), mIncludesLastItem(false)
{
    qRegisterMetaType<EwsSyncFolderItemsRequest::Change::List>();
    qRegisterMetaType<EwsItem>();
}

EwsSyncFolderItemsRequest::~EwsSyncFolderItemsRequest()
{
}

void EwsSyncFolderItemsRequest::setFolderId(const EwsId &id)
{
    mFolderId = id;
}

void EwsSyncFolderItemsRequest::setItemShape(const EwsItemShape &shape)
{
    mShape = shape;
}

void EwsSyncFolderItemsRequest::setSyncState(const QString &state)
{
    mSyncState = state;
}

void EwsSyncFolderItemsRequest::setMaxChanges(uint max)
{
    mMaxChanges = max;
}

void EwsSyncFolderItemsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderItems"));

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderId"));
    mFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    if (!mSyncState.isNull()) {
        writer.writeTextElement(ewsMsgNsUri, QStringLiteral("SyncState"), mSyncState);
    }

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("MaxChangesReturned"),
        QString::number(mMaxChanges));

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        QString st = mSyncState.isNull() ? QStringLiteral("none") : QString::number(qHash(mSyncState), 36);
        QString folder;
        qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting SyncFolderItems request (folder: ")
                        << mFolderId << QStringLiteral(", state: %1").arg(st);
    }

    prepare(reqString);

    doSend();
}

bool EwsSyncFolderItemsRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("SyncFolderItems"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsSyncFolderItemsRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    EwsSyncFolderItemsRequest::Response *resp = new EwsSyncFolderItemsRequest::Response(reader);
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mChanges = resp->mChanges;
    mIncludesLastItem = resp->mIncludesLastItem;

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp->isSuccess()) {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got SyncFolderItems response (%1 changes, last included: %2, state: %3)")
                            .arg(mChanges.size()).arg(mIncludesLastItem ? QStringLiteral("true") : QStringLiteral("false"))
                            .arg(qHash(mSyncState), 0, 36);
        }
        else {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got SyncFolderItems response - %1")
                            .arg(resp->responseMessage());
        }
    }

    return true;
}

EwsSyncFolderItemsRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    static const QVector<EwsXmlReader<SyncResponseElementType>::Item> items = {
        {SyncState, QStringLiteral("SyncState"), &ewsXmlTextReader},
        {IncludesLastItemInRange, QStringLiteral("IncludesLastItemInRange"), &ewsXmlBoolReader},
        {Changes, QStringLiteral("Changes"), &EwsSyncFolderItemsRequest::Response::changeReader},
    };
    static const EwsXmlReader<SyncResponseElementType> staticReader(items);

    EwsXmlReader<SyncResponseElementType> ewsReader(staticReader);

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

    QHash<SyncResponseElementType, QVariant> values = ewsReader.values();

    mSyncState = values[SyncState].toString();
    mIncludesLastItem = values[IncludesLastItemInRange].toBool();
    mChanges = values[Changes].value<Change::List>();
}

bool EwsSyncFolderItemsRequest::Response::changeReader(QXmlStreamReader &reader, QVariant &val)
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

EwsSyncFolderItemsRequest::Change::Change(QXmlStreamReader &reader)
{
    static const QVector<EwsXmlReader<ChangeElementType>::Item> items = {
        {Item, QStringLiteral("Item"), &ewsXmlItemReader},
        {Item, QStringLiteral("Message"), &ewsXmlItemReader},
        {Item, QStringLiteral("CalendarItem"), &ewsXmlItemReader},
        {Item, QStringLiteral("Contact"), &ewsXmlItemReader},
        {Item, QStringLiteral("DistributionList"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingMessage"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingRequest"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingResponse"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingCancellation"), &ewsXmlItemReader},
        {Item, QStringLiteral("Task"), &ewsXmlItemReader},
        {ItemId, QStringLiteral("ItemId"), &ewsXmlIdReader},
        {IsRead, QStringLiteral("IsRead"), &ewsXmlBoolReader}
    };
    static const EwsXmlReader<ChangeElementType> staticReader(items);

    EwsXmlReader<ChangeElementType> ewsReader(staticReader);

    if (reader.name() == QStringLiteral("Create")) {
        mType = Create;
    }
    else if (reader.name() == QStringLiteral("Update")) {
        mType = Update;
    }
    else if (reader.name() == QStringLiteral("Delete")) {
        mType = Delete;
    }
    else if (reader.name() == QStringLiteral("ReadFlagChange")) {
        mType = ReadFlagChange;
    }
    if (!ewsReader.readItems(reader, ewsTypeNsUri)) {
        return;
    }

    QHash<ChangeElementType, QVariant> values = ewsReader.values();

    switch (mType) {
    case Create:
    case Update:
        mItem = values[Item].value<EwsItem>();
        break;
    case ReadFlagChange:
        mIsRead = values[IsRead].toBool();
        /* no break */
    case Delete:
        mId = values[ItemId].value<EwsId>();
        break;
    default:
        break;
    }
}
