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

#include "ewsfetchitemsjob.h"

#include <KMime/Message>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>

#include "ewsfinditemrequest.h"
#include "ewsclient.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

EwsFetchItemsJob::EwsFetchItemsJob(const Collection &collection, EwsClient &client, QObject *parent)
    : EwsJob(parent), mCollection(collection), mClient(client), mJobsFinished(0)
{
    qCDebugNC(EWSCLIENT_LOG) << mCollection.name();
}

EwsFetchItemsJob::~EwsFetchItemsJob()
{
}

void EwsFetchItemsJob::start()
{
    EwsFindItemRequest *findItemsReq = new EwsFindItemRequest(mClient, this);
    findItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
    EwsItemShape shape(EwsShapeIdOnly);
    findItemsReq->setItemShape(shape);
    connect(findItemsReq, SIGNAL(finished(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
    addSubjob(findItemsReq);
    findItemsReq->start();

    ItemFetchJob *itemJob = new ItemFetchJob(mCollection);
    ItemFetchScope itemScope;
    itemScope.setCacheOnly(true);
    itemScope.fetchFullPayload(false);
    itemJob->setFetchScope(itemScope);
    connect(itemJob, SIGNAL(finished(KJob*)), SLOT(localItemFetchDone(KJob*)));
    addSubjob(itemJob);
    itemJob->start();
}

void EwsFetchItemsJob::localItemFetchDone(KJob *job)
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>(job);

    if (!fetchJob) {
        setErrorMsg(QStringLiteral("Invalid item fetch job pointer."));
        doKill();
        emitResult();
    }

    if (!fetchJob->error()) {
        mLocalItems = fetchJob->items();
        mJobsFinished++;
        if (mJobsFinished == 2) {
            compareItemLists();
        }
    }
}

void EwsFetchItemsJob::remoteItemFetchDone(KJob *job)
{
    EwsFindItemRequest *itemReq = qobject_cast<EwsFindItemRequest*>(job);

    if (!itemReq) {
        setErrorMsg(QStringLiteral("Invalid find item request pointer."));
        doKill();
        emitResult();
    }

    if (!itemReq->error()) {
        mRemoteItems = itemReq->items();
        mJobsFinished++;
        if (mJobsFinished == 2) {
            compareItemLists();
        }
    }
}

void EwsFetchItemsJob::compareItemLists()
{
    QHash<QString, EwsItem> remoteIds;

    Q_FOREACH(const EwsItem &item, mRemoteItems) {
        EwsId id = item[EwsItemFieldItemId].value<EwsId>();
        remoteIds.insert(id.id(), item);
    }

    Q_FOREACH(Item item, mLocalItems) {
        QHash<QString, EwsItem>::iterator it = remoteIds.find(item.remoteId());
        if (it != remoteIds.end()) {
            EwsId id = it.value()[EwsItemFieldItemId].value<EwsId>();
            if (id.changeKey() != item.remoteRevision()) {
                item.clearPayload();
                item.setRemoteRevision(id.changeKey());
                mChangedItems.append(item);
            }
            remoteIds.erase(it);
        }
        else {
            mDeletedItems.append(item);
        }
    }

    for (QHash<QString, EwsItem>::const_iterator it = remoteIds.cbegin();
        it != remoteIds.cend(); it++) {
        QString mimeType;
        switch (it->type()) {
        case EwsItemTypeMessage:
        case EwsItemTypeMeetingMessage:
        case EwsItemTypeMeetingRequest:
        case EwsItemTypeMeetingResponse:
        case EwsItemTypeMeetingCancellation:
            mimeType = KMime::Message::mimeType();
            break;
        case EwsItemTypeContact:
            mimeType = KContacts::Addressee::mimeType();
            break;
        case EwsItemTypeCalendarItem:
            mimeType = KCalCore::Event::eventMimeType();
            break;
        case EwsItemTypeTask:
            mimeType = KCalCore::Todo::todoMimeType();
            break;
        default:
            // No idea what kind of item it is - skip it.
            continue;
        }
        Item item(mimeType);
        item.setParentCollection(mCollection);
        EwsId id = it.value()[EwsItemFieldItemId].value<EwsId>();
        item.setRemoteId(id.id());
        item.setRemoteRevision(id.changeKey());
        mChangedItems.append(item);
    }

    emitResult();
}
