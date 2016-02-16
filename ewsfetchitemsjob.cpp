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

#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>

#include "ewsfinditemrequest.h"
#include "ewssyncfolderitemsrequest.h"
#include "ewsclient.h"
#include "ewsmailbox.h"
#include "ewsitemhandler.h"
#include "ewsfetchitemdetailjob.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static Q_CONSTEXPR int listBatchSize = 100;
static Q_CONSTEXPR int fetchBatchSize = 10;

/**
 * The fetch items job is processed in two stages.
 *
 * The first stage is to query the list of messages on the remote and local sides. For this purpose
 * an EwsSyncFolderItemsRequest is started to retrieve remote items (list of ids only) and an ItemFetchJob
 * is started to fetch local items (from cache only). Both of these jobs are started simultaneously.
 *
 * The second stage begins when both item list query jobs have finished. The goal of this stage is
 * to determine a list of items to fetch more details for. Since the EwsSyncFolderItemsRequest can
 * retrieve incremental information further processing depends on whether the sync request was a
 * full sync (no sync state) or an incremental sync.
 *
 * In case of a full sync both item lists are compared to determine lists of new/changed items and
 * deleted items. For an incremental sync there is no need to compare as the lists of
 * added/changed/deleted items are already given. 'IsRead' flag changes changes are treated
 * specially - the modification is trivial and is performed straight away without fetching item
 * details.
 *
 * The list of new/changed items is then used to perform a second remote request in order to
 * retrieve the details of these items. For e-mail items the second fetch only retrieves the
 * item headers. For other items the full MIME content is fetched.
 */

EwsFetchItemsJob::EwsFetchItemsJob(const Collection &collection, EwsClient &client,
                                   const QString &syncState, QObject *parent)
    : EwsJob(parent), mCollection(collection), mClient(client), mPendingJobs(0), mTotalItems(0),
      mSyncState(syncState), mFullSync(syncState.isNull())
{
    qRegisterMetaType<EwsId::List>();
}

EwsFetchItemsJob::~EwsFetchItemsJob()
{
}

void EwsFetchItemsJob::start()
{
    /* Begin stage 1 - query item list from local and remote side. */
    EwsSyncFolderItemsRequest *syncItemsReq = new EwsSyncFolderItemsRequest(mClient, this);
    syncItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
    EwsItemShape shape(EwsShapeIdOnly);
    syncItemsReq->setItemShape(shape);
    if (!mSyncState.isNull()) {
        syncItemsReq->setSyncState(mSyncState);
    }
    syncItemsReq->setMaxChanges(listBatchSize);
    connect(syncItemsReq, SIGNAL(result(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
    addSubjob(syncItemsReq);

    ItemFetchJob *itemJob = new ItemFetchJob(mCollection);
    ItemFetchScope itemScope;
    itemScope.setCacheOnly(true);
    itemScope.fetchFullPayload(false);
    itemJob->setFetchScope(itemScope);
    connect(itemJob, SIGNAL(result(KJob*)), SLOT(localItemFetchDone(KJob*)));
    addSubjob(itemJob);

    mPendingJobs = 2;
    syncItemsReq->start();
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
        removeSubjob(job);
        mLocalItems = fetchJob->items();
        mPendingJobs--;
        if (mPendingJobs == 0) {
            compareItemLists();
        }
    }
}

void EwsFetchItemsJob::remoteItemFetchDone(KJob *job)
{
    EwsSyncFolderItemsRequest *itemReq = qobject_cast<EwsSyncFolderItemsRequest*>(job);

    qDebug() << "remoteItemFetchDone";
    if (!itemReq) {
        setErrorMsg(QStringLiteral("Invalid find item request pointer."));
        doKill();
        emitResult();
    }

    if (!itemReq->error()) {
        removeSubjob(job);
        itemReq->dump();
        Q_FOREACH(const EwsSyncFolderItemsRequest::Change &change, itemReq->changes()) {
            switch (change.type()) {
            case EwsSyncFolderItemsRequest::Create:
                mRemoteAddedItems.append(change.item());
                break;
            case EwsSyncFolderItemsRequest::Update:
                mRemoteChangedItems.append(change.item());
                break;
            case EwsSyncFolderItemsRequest::Delete:
                mRemoteDeletedIds.append(change.itemId());
                break;
            case EwsSyncFolderItemsRequest::ReadFlagChange:
                mRemoteFlagChangedIds.insert(change.itemId(), change.isRead());
                break;
            default:
                break;
            }
        }

        if (!itemReq->includesLastItem()) {
            EwsSyncFolderItemsRequest *syncItemsReq = new EwsSyncFolderItemsRequest(mClient, this);
            syncItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
            EwsItemShape shape(EwsShapeIdOnly);
            syncItemsReq->setItemShape(shape);
            syncItemsReq->setSyncState(itemReq->syncState());
            syncItemsReq->setMaxChanges(listBatchSize);
            connect(syncItemsReq, SIGNAL(result(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
            addSubjob(syncItemsReq);
            syncItemsReq->start();
            qDebug() << "remoteItemFetchDone: started next batch";
        }
        else {
            mSyncState = itemReq->syncState();
            mPendingJobs--;
            if (mPendingJobs == 0) {
                compareItemLists();
            }
        }
    }
}

void EwsFetchItemsJob::compareItemLists()
{
    /* Begin stage 2 - determine list of new/changed items and fetch details about them. */

    Item::List toFetchItems[EwsItemTypeUnknown + 1];

    Q_EMIT status(1, QStringLiteral("Retrieving items"));
    Q_EMIT percent(0);

    QHash<QString, Item> itemHash;
    Q_FOREACH(const Item& item, mLocalItems) {
        itemHash.insert(item.remoteId(), item);
    }

    Q_FOREACH(const EwsItem &ewsItem, mRemoteAddedItems) {
        /* In case of a full sync all existing items appear as added on the remote side. Therefore
         * look for the item in the local list before creating a new copy. */
        EwsId id(ewsItem[EwsItemFieldItemId].value<EwsId>());
        QHash<QString, Item>::iterator it = itemHash.find(id.id());
        EwsItemType type = ewsItem.internalType();
        QString mimeType = EwsItemHandler::itemHandler(type)->mimeType();
        if (it == itemHash.end()) {
            Item item(mimeType);
            item.setParentCollection(mCollection);
            EwsId id = ewsItem[EwsItemFieldItemId].value<EwsId>();
            item.setRemoteId(id.id());
            item.setRemoteRevision(id.changeKey());
            toFetchItems[type].append(item);
        }
        else {
            Item &item = *it;
            item.clearPayload();
            item.setRemoteRevision(id.changeKey());
            toFetchItems[type].append(item);
            itemHash.erase(it);
        }
    }

    if (mFullSync) {
        /* In case of a full sync all items that are still on the local item list do not exist
         * remotely and need to be deleted locally. */
        QHash<QString, Item>::iterator it;
        for (it = itemHash.begin(); it != itemHash.end(); it++) {
            mDeletedItems.append(it.value());
        }
    }
    else {
        Q_FOREACH(const EwsItem &ewsItem, mRemoteChangedItems) {
            EwsId id(ewsItem[EwsItemFieldItemId].value<EwsId>());
            QHash<QString, Item>::iterator it = itemHash.find(id.id());
            if (it == itemHash.end()) {
                setErrorMsg(QStringLiteral("Got update for item %1, but item not found in local store.")
                                .arg(id.id()));
                emitResult();
                return;
            }
            Item &item = *it;
            item.clearPayload();
            item.setRemoteRevision(id.changeKey());
            EwsItemType type = ewsItem.internalType();
            toFetchItems[type].append(item);
            itemHash.erase(it);
        }

        // In case of an incremental sync deleted items will be given explicitly. */
        Q_FOREACH(const EwsId &id, mRemoteDeletedIds) {
            QHash<QString, Item>::iterator it = itemHash.find(id.id());
            if (it == itemHash.end()) {
                setErrorMsg(QStringLiteral("Got delete for item %1, but item not found in local store.")
                                .arg(id.id()));
                emitResult();
                return;
            }
            mDeletedItems.append(*it);
        }

        QHash<EwsId, bool>::const_iterator it;
        EwsItemHandler *handler = EwsItemHandler::itemHandler(EwsItemTypeMessage);
        for (it = mRemoteFlagChangedIds.cbegin(); it != mRemoteFlagChangedIds.cend(); it++) {
            QHash<QString, Item>::iterator iit = itemHash.find(it.key().id());
            if (iit == itemHash.end()) {
                setErrorMsg(QStringLiteral("Got read flag change for item %1, but item not found in local store.")
                                .arg(it.key().id()));
                emitResult();
                return;
            }
            Item &item = *iit;
            handler->setSeenFlag(item, it.value());
            mChangedItems.append(item);
            itemHash.erase(iit);
        }
    }

    qCDebugNC(EWSRES_LOG) << QStringLiteral("Changed %2, deleted %3, new %4")
                    .arg(mRemoteChangedItems.size())
                    .arg(mDeletedItems.size()).arg(mRemoteAddedItems.size());

    bool fetch = false;
    for (unsigned iType = 0; iType < sizeof(toFetchItems) / sizeof(toFetchItems[0]); iType++) {
        if (!toFetchItems[iType].isEmpty()) {
            qDebug() << "compareItemLists: fetching" << iType;
            for (int i = 0; i < toFetchItems[iType].size(); i += fetchBatchSize) {
                EwsItemHandler *handler = EwsItemHandler::itemHandler(static_cast<EwsItemType>(iType));
                if (!handler) {
                    // TODO: Temporarily ignore unsupported item types.
                    qCWarning(EWSRES_LOG) << QStringLiteral("Unable to initialize fetch for item type %1")
                                    .arg(iType);
                    /*setErrorMsg(QStringLiteral("Unable to initialize fetch for item type %1").arg(iType));
                    emitResult();
                    return;*/
                }
                else {
                    EwsFetchItemDetailJob *job = handler->fetchItemDetailJob(mClient, this, mCollection);
                    Item::List itemList = toFetchItems[iType].mid(i, fetchBatchSize);
                    job->setItemLists(itemList, &mDeletedItems);
                    connect(job, SIGNAL(result(KJob*)), SLOT(itemDetailFetchDone(KJob*)));
                    addSubjob(job);
                    qDebug() << "compareItemLists: job created";
                    fetch = true;
                }
            }
        }
    }
    if (!fetch) {
        // Nothing to fetch - we're done here.
        emitResult();
    }
    else {
        qDebug() << "compareItemLists: jobs" << subjobs().size();
        subjobs().first()->start();
    }
}

void EwsFetchItemsJob::itemDetailFetchDone(KJob *job)
{
    qDebug() << "itemDetailFetchDone";
    removeSubjob(job);

    if (!job->error()) {
        EwsFetchItemDetailJob *detailJob = qobject_cast<EwsFetchItemDetailJob*>(job);
        if (detailJob) {
            mChangedItems += detailJob->changedItems();
        }

        qDebug() << "itemDetailFetchDone: jobs" << subjobs().size();
        if (subjobs().size() == 0) {
            emitResult();
        }
        else {
            subjobs().first()->start();
        }
    }
}
