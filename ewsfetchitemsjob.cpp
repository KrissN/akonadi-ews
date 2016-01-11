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

#include <QtCore/QTimeZone>
#include <KMime/Message>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include "ewsfinditemrequest.h"
#include "ewsclient.h"
#include "ewsmailbox.h"
#include "ewsfetchitemdetailjob.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static Q_CONSTEXPR int listBatchSize = 100;
static Q_CONSTEXPR int fetchBatchSize = 10;

/**
 * The fetch items job is processed in two stages.
 *
 * The first stage is to query the list of messages on the remote and local sides. For this purpose
 * an EwsFindItemsRequest is started to retrieve remote items (list of ids only) and an ItemFetchJob
 * is started to fetch local items (from cache only). Both of these jobs are started simultaneously.
 *
 * The second stage begins when both item list query jobs have finished. Both item lists are
 * compared to determine lists of new/changed items and deleted items. The list of new/changed items
 * is then used to perform a second remote request in order to retrieve the details of these items.
 * For e-mail items the second fetch only retrieves the item headers. For other items the full
 * MIME content is fetched.
 */

EwsFetchItemsJob::EwsFetchItemsJob(const Collection &collection, EwsClient &client, QObject *parent)
    : EwsJob(parent), mCollection(collection), mClient(client), mPendingJobs(0), mTotalItems(0)
{
    qCDebugNC(EWSCLIENT_LOG) << mCollection.name();

    qRegisterMetaType<EwsId::List>();
}

EwsFetchItemsJob::~EwsFetchItemsJob()
{
}

void EwsFetchItemsJob::start()
{
    /* Begin stage 1 - query item list from local and remote side. */
    EwsFindItemRequest *findItemsReq = new EwsFindItemRequest(mClient, this);
    findItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
    EwsItemShape shape(EwsShapeIdOnly);
    findItemsReq->setItemShape(shape);
    /* Use paged listing instead of querying the full list at once. This not only prevents long
     * single requests going through the network but is necessary to properly retrieve calendar
     * items.
     *
     * According to EWS documentation listing calendar items behaves differently for recurring items
     * depending on the usage of paging. Without paging the listing will return all occurrences of
     * recurring items, while a paged view will only return recurring masters and exceptions.
     *
     * https://msdn.microsoft.com/EN-US/library/office/dn727655(v=exchg.150).aspx
     */
    findItemsReq->setPagination(EwsBasePointBeginning, 0, listBatchSize);
    connect(findItemsReq, SIGNAL(result(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
    addSubjob(findItemsReq);

    ItemFetchJob *itemJob = new ItemFetchJob(mCollection);
    ItemFetchScope itemScope;
    itemScope.setCacheOnly(true);
    itemScope.fetchFullPayload(false);
    itemJob->setFetchScope(itemScope);
    connect(itemJob, SIGNAL(result(KJob*)), SLOT(localItemFetchDone(KJob*)));
    addSubjob(itemJob);

    mPendingJobs = 2;
    findItemsReq->start();
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
    EwsFindItemRequest *itemReq = qobject_cast<EwsFindItemRequest*>(job);

    qDebug() << "remoteItemFetchDone";
    if (!itemReq) {
        setErrorMsg(QStringLiteral("Invalid find item request pointer."));
        doKill();
        emitResult();
    }

    if (!itemReq->error()) {
        removeSubjob(job);
        mRemoteItems += itemReq->items();

        if (!itemReq->includesLastItem()) {
            // More items to come - start a request for the next batch.
            EwsFindItemRequest *findItemsReq = new EwsFindItemRequest(mClient, this);
            findItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
            EwsItemShape shape(EwsShapeIdOnly);
            findItemsReq->setItemShape(shape);
            findItemsReq->setPagination(EwsBasePointBeginning, itemReq->nextOffset(), listBatchSize);
            connect(findItemsReq, SIGNAL(result(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
            addSubjob(findItemsReq);
            findItemsReq->start();
            qDebug() << "remoteItemFetchDone: started next batch";
        }
        else {
            qDebug() << "remoteItemFetchDone: done";
            mPendingJobs--;
            if (mPendingJobs == 0) {
                compareItemLists();
            }
        }
    }
}

void EwsFetchItemsJob::compareItemLists()
{
    qDebug() << "compareItemLists: start";
    /* Begin stage 2 - determine list of new/changed items and fetch details about them. */
    QHash<QString, EwsItem> remoteIds;
    bool fetch = false;

    Q_EMIT status(1, QStringLiteral("Retrieving items"));
    Q_EMIT percent(0);

    Q_FOREACH(const EwsItem &item, mRemoteItems) {
        EwsId id = item[EwsItemFieldItemId].value<EwsId>();
        remoteIds.insert(id.id(), item);
    }

    Item::List toFetchItems[EwsItemTypeUnknown + 1];

    int unchanged = 0;

    Q_FOREACH(Item item, mLocalItems) {
        QHash<QString, EwsItem>::iterator it = remoteIds.find(item.remoteId());
        if (it != remoteIds.end()) {
            EwsId id = it.value()[EwsItemFieldItemId].value<EwsId>();
            qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Found match with existing item.") << id.id();
            if (id.changeKey() != item.remoteRevision()) {
                qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Found changed item.") << id.id();
                item.clearPayload();
                item.setRemoteRevision(id.changeKey());
                EwsItemType type = it->type();
                switch (type) {
                case EwsItemTypeMessage:
                case EwsItemTypeMeetingMessage:
                case EwsItemTypeMeetingRequest:
                case EwsItemTypeMeetingResponse:
                case EwsItemTypeMeetingCancellation:
                    type = EwsItemTypeMessage;
                    break;
                default:
                    break;
                }
                qDebug() << "changed item type" << type;
                toFetchItems[type].append(item);
                fetch = true;
            }
            else {
                unchanged++;
            }
            remoteIds.erase(it);
        }
        else {
            qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Existing item not found - deleting.") << item.remoteId();
            mDeletedItems.append(item);
        }
    }

    for (QHash<QString, EwsItem>::const_iterator it = remoteIds.cbegin();
        it != remoteIds.cend(); it++) {
        QString mimeType;
        EwsItemType type = it->type();
        switch (type) {
        case EwsItemTypeMessage:
        case EwsItemTypeMeetingMessage:
        case EwsItemTypeMeetingRequest:
        case EwsItemTypeMeetingResponse:
        case EwsItemTypeMeetingCancellation:
            mimeType = KMime::Message::mimeType();
            type = EwsItemTypeMessage;
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
        qDebug() << "new item type" << type;
        toFetchItems[type].append(item);
        fetch = true;
    }

    for (unsigned iType = 0; iType < sizeof(toFetchItems) / sizeof(toFetchItems[0]); iType++) {
        qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Unchanged %1, changed %2, deleted %3, new %4")
                        .arg(unchanged).arg(toFetchItems[iType].size())
                        .arg(mDeletedItems.size()).arg(remoteIds.size());
    }
    qDebug() << "compareItemLists";

    for (unsigned iType = 0; iType < sizeof(toFetchItems) / sizeof(toFetchItems[0]); iType++) {
        if (!toFetchItems[iType].isEmpty()) {
            qDebug() << "compareItemLists: fetching" << iType;
            for (int i = 0; i < toFetchItems[iType].size(); i += fetchBatchSize) {
                EwsFetchItemDetailJob *job =
                    EwsFetchItemDetailJob::createFetchItemDetailJob(static_cast<EwsItemType>(iType),
                                                                    mClient, this, mCollection);
                if (!job) {
                    setErrorMsg(QStringLiteral("Unable to initialize fetch for item type %1").arg(iType));
                    emitResult();
                    return;
                }
                Item::List itemList = toFetchItems[iType].mid(i, fetchBatchSize);
                job->setItemLists(itemList, &mDeletedItems);
                connect(job, SIGNAL(result(KJob*)), SLOT(itemDetailFetchDone(KJob*)));
                addSubjob(job);
                qDebug() << "compareItemLists: job created";
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
