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

#include <QtCore/QDebug>

#include <KI18n/KLocalizedString>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/CollectionFetchJob>
#include <KMime/Message>
#include <KWallet/KWallet>
#include <KWidgetsAddons/KPasswordDialog>

#include "ewsresource.h"
#include "ewsfetchitemsjob.h"
#include "ewsfetchfoldersjob.h"
#include "ewsgetitemrequest.h"
#include "ewsupdateitemrequest.h"
#include "ewsmoveitemrequest.h"
#include "ewsdeleteitemrequest.h"
#include "ewscreatefolderrequest.h"
#include "ewsmovefolderrequest.h"
#include "ewsupdatefolderrequest.h"
#include "ewsdeletefolderrequest.h"
#include "ewssubscriptionmanager.h"
#include "ewsgetfolderrequest.h"
#include "ewsitemhandler.h"
#include "ewsmodifyitemjob.h"
#include "ewscreateitemjob.h"
#include "configdialog.h"
#include "settings.h"
#ifdef HAVE_SEPARATE_MTA_RESOURCE
#include "ewscreateitemrequest.h"
#endif
#include "ewsclient_debug.h"

#include "resourceadaptor.h"

using namespace Akonadi;

EwsResource::EwsResource(const QString &id)
    : Akonadi::ResourceBase(id)
{
    qDebug() << "EwsResource";
    //setName(i18n("Microsoft Exchange"));
    mEwsClient.setUrl(Settings::baseUrl());
    requestPassword(mPassword, true);
    if (Settings::domain().isEmpty()) {
        mEwsClient.setCredentials(Settings::username(), mPassword);
    } else {
        mEwsClient.setCredentials(Settings::domain() + '\\' + Settings::username(), mPassword);
    }

    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);

    mRootCollection.setParentCollection(Collection::root());
    mRootCollection.setName(name());
    mRootCollection.setContentMimeTypes(QStringList() << Collection::mimeType() << KMime::Message::mimeType());
    mRootCollection.setRights(Collection::ReadOnly);

    if (Settings::baseUrl().isEmpty()) {
        setOnline(false);
        Q_EMIT status(NotConfigured, i18nc("@info:status", "No server configured yet."));
    } else {
        resetUrl();
    }

    // Load the sync state
    QByteArray data = QByteArray::fromBase64(Settings::self()->syncState().toAscii());
    if (!data.isEmpty()) {
        data = qUncompress(data);
        if (!data.isEmpty()) {
            QDataStream stream(data);
            stream >> mSyncState;
            qDebug() << mSyncState;
        }
    }

    QMetaObject::invokeMethod(this, "delayedInit", Qt::QueuedConnection);
}

EwsResource::~EwsResource()
{
}

void EwsResource::delayedInit()
{
    new ResourceAdaptor(this);
}

void EwsResource::resetUrl()
{
    Q_EMIT status(Running, i18nc("@info:status", "Connecting to Exchange server"));

    EwsGetFolderRequest *req = new EwsGetFolderRequest(mEwsClient, this);
    req->setFolderIds(EwsId::List() << EwsId(EwsDIdMsgFolderRoot));
    EwsFolderShape shape(EwsShapeIdOnly);
    shape << EwsPropertyField(QStringLiteral("folder:DisplayName"));
    req->setFolderShape(shape);
    connect(req, &EwsRequest::result, this, &EwsResource::rootFolderFetchFinished);
    req->start();
}

void EwsResource::rootFolderFetchFinished(KJob *job)
{
    qDebug() << "rootFolderFetchFinished";
    EwsGetFolderRequest *req = qobject_cast<EwsGetFolderRequest*>(job);
    if (!req) {
        Q_EMIT status(Broken, i18nc("@info:status", "Unable to connect to Exchange server"));
        setOnline(false);
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsGetFolderRequest job object");
        return;
    }

    if (req->error()) {
        Q_EMIT status(Broken, i18nc("@info:status", "Unable to connect to Exchange server"));
        setOnline(false);
        qWarning() << "ERROR" << req->errorString();
        return;
    }

    if (req->responses().size() != 1) {
        Q_EMIT status(Broken, i18nc("@info:status", "Unable to connect to Exchange server"));
        setOnline(false);
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid number of responses received");
        return;
    }

    EwsId id = req->responses().first().folder()[EwsFolderFieldFolderId].value<EwsId>();
    if (id.type() == EwsId::Real) {
        mRootCollection.setRemoteId(id.id());
        mRootCollection.setRemoteRevision(id.changeKey());
        qDebug() << "Root folder is " << id;
        Q_EMIT status(Idle, i18nc("@info:status", "Ready"));

        mSubManager.reset(new EwsSubscriptionManager(mEwsClient, id, this));
        connect(mSubManager.data(), &EwsSubscriptionManager::foldersModified, this, &EwsResource::foldersModifiedEvent);
        connect(mSubManager.data(), &EwsSubscriptionManager::folderTreeModified, this, &EwsResource::folderTreeModifiedEvent);
        connect(mSubManager.data(), &EwsSubscriptionManager::fullSyncRequested, this, &EwsResource::fullSyncRequestedEvent);
        mSubManager->start();
    }
}

void EwsResource::retrieveCollections()
{
    qDebug() << "retrieveCollections";
    if (mRootCollection.remoteId().isNull()) {
        cancelTask(QStringLiteral("Root folder id not known."));
        return;
    }

    Q_EMIT status(Running, i18nc("@info:status", "Retrieving collection tree"));

    EwsFetchFoldersJob *job = new EwsFetchFoldersJob(mEwsClient, mFolderSyncState,
        mRootCollection, this);
    connect(job, &EwsFetchFoldersJob::result, this, &EwsResource::findFoldersRequestFinished);
    job->start();
}

void EwsResource::retrieveItems(const Collection &collection)
{
    Q_EMIT status(1, QStringLiteral("Retrieving item list"));
    qDebug() << "retrieveItems";

    Q_EMIT status(Running, i18nc("@info:status", "Retrieving %1 items").arg(collection.name()));

    QString rid = collection.remoteId();
    EwsFetchItemsJob *job = new EwsFetchItemsJob(collection, mEwsClient,
        mSyncState.value(rid), mItemsToCheck.value(rid), this);
    job->setQueuedUpdates(mQueuedUpdates.value(collection.remoteId()));
    mQueuedUpdates.remove(collection.remoteId());
    connect(job, SIGNAL(finished(KJob*)), SLOT(itemFetchJobFinished(KJob*)));
    connect(job, SIGNAL(status(int,const QString&)), SIGNAL(status(int,const QString&)));
    connect(job, SIGNAL(percent(int)), SIGNAL(percent(int)));
    job->start();
}

bool EwsResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    qDebug() << "retrieveItem";
    EwsGetItemRequest *req = new EwsGetItemRequest(mEwsClient, this);
    EwsId::List ids;
    ids << EwsId(item.remoteId(), item.remoteRevision());
    req->setItemIds(ids);
    EwsItemShape shape(EwsShapeIdOnly);
    shape << EwsPropertyField("item:MimeContent");
    req->setItemShape(shape);
    req->setProperty("item", QVariant::fromValue<Item>(item));
    connect(req, &EwsGetItemRequest::result, req,
            [this](KJob *job){getItemRequestFinished(qobject_cast<EwsGetItemRequest*>(job));});
    req->start();
    return true;
}

void EwsResource::configure(WId windowId)
{
    ConfigDialog dlg(this, mEwsClient, windowId);
    if (dlg.exec()) {
        mSubManager.reset(Q_NULLPTR);
        mEwsClient.setUrl(Settings::baseUrl());
        if (Settings::domain().isEmpty()) {
            mEwsClient.setCredentials(Settings::username(), mPassword);
        } else {
            mEwsClient.setCredentials(Settings::domain() + '\\' + Settings::username(), mPassword);
        }
        Settings::self()->save();
        resetUrl();
    }
}

void EwsResource::findFoldersRequestFinished(KJob *job)
{
    qDebug() << "findFoldersRequestFinished";
    Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
    EwsFetchFoldersJob *req = qobject_cast<EwsFetchFoldersJob*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsFetchFoldersJob job object");
        cancelTask(QStringLiteral("Invalid EwsFetchFoldersJob job object"));
        return;
    }

    if (req->error()) {
        qWarning() << "ERROR" << req->errorString();
        cancelTask(req->errorString());
        return;
    }

    mFolderSyncState = req->syncState();
    if (req->fullSync()) {
        collectionsRetrieved(req->folders());
    }
    else {
        collectionsRetrievedIncremental(req->changedFolders(), req->deletedFolders());
    }
}

void EwsResource::itemFetchJobFinished(KJob *job)
{
    EwsFetchItemsJob *fetchJob = qobject_cast<EwsFetchItemsJob*>(job);

    if (!fetchJob) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid EwsFetchItemsJobjob object");
        cancelTask(QStringLiteral("Invalid EwsFetchItemsJob job object"));
        return;
    }
    if (job->error()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Item fetch error:") << job->errorString();
        if (mSyncState.contains(fetchJob->collection().remoteId())) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Retrying with empty state.");
            // Retry with a clear sync state.
            mSyncState.remove(fetchJob->collection().remoteId());
            retrieveItems(fetchJob->collection());
        }
        else {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Clean sync failed.");
            // No more hope
            cancelTask(job->errorString());
            return;
        }
    }
    else {
        mSyncState[fetchJob->collection().remoteId()] = fetchJob->syncState();
        itemsRetrievedIncremental(fetchJob->changedItems(), fetchJob->deletedItems());
    }
    saveState();
    mItemsToCheck.remove(fetchJob->collection().remoteId());
    Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
}

void EwsResource::getItemRequestFinished(EwsGetItemRequest *req)
{
    qDebug() << "getItemRequestFinished";

    if (req->error()) {
        qWarning() << "ERROR" << req->errorString();
        cancelTask(req->errorString());
        return;
    }

    Item item = req->property("item").value<Item>();
    const EwsGetItemRequest::Response &resp = req->responses()[0];
    if (!resp.isSuccess()) {
        qWarning() << QStringLiteral("Item fetch failed!");
        cancelTask(QStringLiteral("Item fetch failed!"));
        return;
    }
    const EwsItem &ewsItem = resp.item();
    if (!EwsItemHandler::itemHandler(ewsItem.internalType())->setItemPayload(item, ewsItem)) {
        cancelTask(QStringLiteral("Failed to fetch item payload."));
        return;
    }

    itemRetrieved(item);
}

void EwsResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    EwsItemType type = EwsItemHandler::mimeToItemType(item.mimeType());
    if (type == EwsItemTypeItem) {
        cancelTask("Item type not supported for changing");
    }
    else {
        EwsModifyItemJob *job = EwsItemHandler::itemHandler(type)->modifyItemJob(mEwsClient, item,
            partIdentifiers, this);
        connect(job, SIGNAL(result(KJob*)), SLOT(itemChangeRequestFinished(KJob*)));
        job->start();
    }
}

void EwsResource::itemChangeRequestFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsModifyItemJob *req = qobject_cast<EwsModifyItemJob*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsModifyItemJob job object"));
        return;
    }

    changeCommitted(req->item());
}

void EwsResource::itemsMoved(const Item::List &items, const Collection &sourceCollection,
                             const Collection &destinationCollection)
{
    EwsId::List ids;

    Q_FOREACH(const Item &item, items) {
        EwsId id(item.remoteId(), item.remoteRevision());
        ids.append(id);
    }

    EwsMoveItemRequest *req = new EwsMoveItemRequest(mEwsClient, this);
    req->setItemIds(ids);
    EwsId destId(destinationCollection.remoteId(), QString());
    req->setDestinationFolderId(destId);
    req->setProperty("items", QVariant::fromValue<Item::List>(items));
    req->setProperty("sourceCollection", QVariant::fromValue<Collection>(sourceCollection));
    req->setProperty("destinationCollection", QVariant::fromValue<Collection>(destinationCollection));
    connect(req, SIGNAL(result(KJob*)), SLOT(itemMoveRequestFinished(KJob*)));
    req->start();
}

void EwsResource::itemMoveRequestFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsMoveItemRequest *req = qobject_cast<EwsMoveItemRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsMoveItemRequest job object"));
        return;
    }
    Item::List items = job->property("items").value<Item::List>();

    if (items.count() != req->responses().count()) {
        cancelTask(QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    /* When moving a batch of items it is possible that the operation will fail for some of them.
     * Unfortunately Akonadi doesn't provide a way to report such partial success/failure. In order
     * to work around this in case of partial failure the source and destination folders will be
     * resynchronised. In order to avoid doing a full sync a hint will be provided in order to
     * indicate the item(s) to check.
     */

    Item::List movedItems;
    EwsId::List failedIds;

    Collection srcCol = req->property("sourceCollection").value<Collection>();
    Collection dstCol = req->property("destinationCollection").value<Collection>();
    Item::List::iterator it = items.begin();
    Q_FOREACH(const EwsMoveItemRequest::Response &resp, req->responses()) {
        Item &item = *it;
        if (resp.isSuccess()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Move succeeded for item %1 %2").arg(resp.itemId().id()).arg(item.remoteId());
            if (item.isValid()) {
                /* Log item deletion in the source folder so that the next sync doesn't trip over
                 * non-existent items. Use old remote ids for that. */
                if (mSubManager) {
                    mSubManager->queueUpdate(EwsDeletedEvent, item.remoteId(), QString());
                }
                mQueuedUpdates[srcCol.remoteId()].append({item.remoteId(), QString(), EwsDeletedEvent});

                item.setRemoteId(resp.itemId().id());
                item.setRemoteRevision(resp.itemId().changeKey());
                movedItems.append(item);
            }
        }
        else {
            warning(QStringLiteral("Move failed for item %1").arg(item.remoteId()));
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Move failed for item %1").arg(item.remoteId());
            failedIds.append(EwsId(item.remoteId(), QString()));
        }
        it++;
    }

    if (!failedIds.isEmpty()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to move %1 items. Forcing src & dst folder sync.")
                        .arg(failedIds.size());
        mItemsToCheck[srcCol.remoteId()] += failedIds;
        foldersModifiedEvent(EwsId::List({EwsId(srcCol.remoteId(), QString())}));
        mItemsToCheck[dstCol.remoteId()] += failedIds;
        foldersModifiedEvent(EwsId::List({EwsId(dstCol.remoteId(), QString())}));
    }

    changesCommitted(movedItems);
}

void EwsResource::itemsRemoved(const Item::List &items)
{
    EwsId::List ids;

    Q_FOREACH(const Item &item, items) {
        EwsId id(item.remoteId(), item.remoteRevision());
        ids.append(id);
    }

    EwsDeleteItemRequest *req = new EwsDeleteItemRequest(mEwsClient, this);
    req->setItemIds(ids);
    req->setProperty("items", QVariant::fromValue<Item::List>(items));
    connect(req, &EwsDeleteItemRequest::result, this, &EwsResource::itemDeleteRequestFinished);
    req->start();

}

void EwsResource::itemDeleteRequestFinished(KJob *job)
{
    qDebug() << "itemDeleteRequestFinished";
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsDeleteItemRequest *req = qobject_cast<EwsDeleteItemRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsDeleteItemRequest job object"));
        return;
    }
    Item::List items = job->property("items").value<Item::List>();

    if (items.count() != req->responses().count()) {
        cancelTask(QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    /* When removing a batch of items it is possible that the operation will fail for some of them.
     * Unfortunately Akonadi doesn't provide a way to report such partial success/failure. In order
     * to work around this in case of partial failure the original folder(s) will be resynchronised.
     * In order to avoid doing a full sync a hint will be provided in order to indicate the item(s)
     * to check.
     */

    EwsId::List foldersToSync;

    Item::List::iterator it = items.begin();
    Q_FOREACH(const EwsDeleteItemRequest::Response &resp, req->responses()) {
        Item &item = *it;
        if (resp.isSuccess()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Delete succeeded for item %1").arg(item.remoteId());
            if (mSubManager) {
                mSubManager->queueUpdate(EwsDeletedEvent, item.remoteId(), QString());
            }
            mQueuedUpdates[item.parentCollection().remoteId()].append({item.remoteId(), QString(), EwsDeletedEvent});
        }
        else {
            warning(QStringLiteral("Delete failed for item %1").arg(item.remoteId()));
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Delere failed for item %1").arg(item.remoteId());
            EwsId colId = EwsId(item.parentCollection().remoteId(), QString());
            mItemsToCheck[colId.id()].append(EwsId(item.remoteId(), QString()));
            if (!foldersToSync.contains(colId)) {
                foldersToSync.append(colId);
            }
        }
        it++;
    }

    if (!foldersToSync.isEmpty()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Need to force sync for %1 folders.")
                        .arg(foldersToSync.size());
        foldersModifiedEvent(foldersToSync);
    }

    changeProcessed();
}

void EwsResource::itemAdded(const Item &item, const Collection &collection)
{
    EwsItemType type = EwsItemHandler::mimeToItemType(item.mimeType());
    if (type == EwsItemTypeItem) {
        cancelTask("Item type not supported for creation");
    }
    else {
        EwsCreateItemJob *job = EwsItemHandler::itemHandler(type)->createItemJob(mEwsClient, item,
            collection, this);
        connect(job, &EwsCreateItemJob::result, this, &EwsResource::itemCreateRequestFinished);
        job->start();
    }
}

void EwsResource::itemCreateRequestFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsCreateItemJob *req = qobject_cast<EwsCreateItemJob*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsCreateItemJob job object"));
        return;
    }

    changeCommitted(req->item());
}

void EwsResource::collectionAdded(const Collection &collection, const Collection &parent)
{
    qDebug() << "collectionAdded";
    EwsFolderType type;
    QStringList mimeTypes = collection.contentMimeTypes();
    if (mimeTypes.contains(EwsItemHandler::itemHandler(EwsItemTypeCalendarItem)->mimeType())) {
        type = EwsFolderTypeCalendar;
    }
    else if (mimeTypes.contains(EwsItemHandler::itemHandler(EwsItemTypeContact)->mimeType())) {
        type = EwsFolderTypeContacts;
    }
    else if (mimeTypes.contains(EwsItemHandler::itemHandler(EwsItemTypeTask)->mimeType())) {
        type = EwsFolderTypeTasks;
    }
    else if (mimeTypes.contains(EwsItemHandler::itemHandler(EwsItemTypeMessage)->mimeType())) {
        type = EwsFolderTypeMail;
    }
    else {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Cannot determine EWS folder type.");
        cancelTask(i18n("Cannot determine EWS folder type."));
        return;
    }

    EwsFolder folder;
    folder.setType(type);
    folder.setField(EwsFolderFieldDisplayName, collection.name());

    EwsCreateFolderRequest *req = new EwsCreateFolderRequest(mEwsClient, this);
    req->setParentFolderId(EwsId(parent.remoteId()));
    req->setFolders(EwsFolder::List() << folder);
    req->setProperty("collection", QVariant::fromValue<Collection>(collection));
    connect(req, &EwsCreateFolderRequest::result, this, &EwsResource::folderCreateRequestFinished);
    req->start();
}

void EwsResource::folderCreateRequestFinished(KJob *job)
{
    qDebug() << "folderCreateRequestFinished";
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsCreateFolderRequest *req = qobject_cast<EwsCreateFolderRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsCreateFolderRequest job object"));
        return;
    }
    Collection col = job->property("collection").value<Collection>();

    EwsCreateFolderRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        const EwsId &id = resp.folderId();
        col.setRemoteId(id.id());
        col.setRemoteRevision(id.changeKey());
        changeCommitted(col);
    }
    else {
        cancelTask(i18n("Failed to create folder"));
    }
}

void EwsResource::collectionMoved(const Collection &collection, const Collection &collectionSource,
                                 const Collection &collectionDestination)
{
    Q_UNUSED(collectionSource)

    EwsId::List ids;
    ids.append(EwsId(collection.remoteId(), collection.remoteRevision()));

    EwsMoveFolderRequest *req = new EwsMoveFolderRequest(mEwsClient, this);
    req->setFolderIds(ids);
    EwsId destId(collectionDestination.remoteId());
    req->setDestinationFolderId(destId);
    req->setProperty("collection", QVariant::fromValue<Collection>(collection));
    connect(req, &EwsMoveFolderRequest::result, this, &EwsResource::folderMoveRequestFinished);
    req->start();
}

void EwsResource::folderMoveRequestFinished(KJob *job)
{
    qDebug() << "folderMoveRequestFinished";
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsMoveFolderRequest *req = qobject_cast<EwsMoveFolderRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsMoveFolderRequest job object"));
        return;
    }
    Collection col = job->property("collection").value<Collection>();

    if (req->responses().count() != 1) {
        cancelTask(QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    EwsMoveFolderRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        const EwsId &id = resp.folderId();
        col.setRemoteId(id.id());
        col.setRemoteRevision(id.changeKey());
        changeCommitted(col);
    }
    else {
        cancelTask(i18n("Failed to move folder"));
    }
}

void EwsResource::collectionChanged(const Collection &collection,
                                    const QSet<QByteArray> &changedAttributes)
{
    qDebug() << "collectionChanged" << collection.name() << changedAttributes;

    if (changedAttributes.contains("NAME")) {
        EwsUpdateFolderRequest *req = new EwsUpdateFolderRequest(mEwsClient, this);
        EwsUpdateFolderRequest::FolderChange fc(EwsId(collection.remoteId(), collection.remoteRevision()),
                                                EwsFolderTypeMail);
        EwsUpdateFolderRequest::Update *upd
            = new EwsUpdateFolderRequest::SetUpdate(EwsPropertyField(QStringLiteral("folder:DisplayName")),
                                                    collection.name());
        fc.addUpdate(upd);
        req->addFolderChange(fc);
        req->setProperty("collection", QVariant::fromValue<Collection>(collection));
        connect(req, &EwsUpdateFolderRequest::finished, this, &EwsResource::folderUpdateRequestFinished);
        req->start();
    } else {
        changeCommitted(collection);
    }
}

void EwsResource::collectionChanged(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection)
}

void EwsResource::folderUpdateRequestFinished(KJob *job)
{
    qDebug() << "folderUpdateRequestFinished";
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsUpdateFolderRequest *req = qobject_cast<EwsUpdateFolderRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsUpdateFolderRequest job object"));
        return;
    }
    Collection col = job->property("collection").value<Collection>();

    if (req->responses().count() != 1) {
        cancelTask(QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    EwsUpdateFolderRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        const EwsId &id = resp.folderId();
        col.setRemoteId(id.id());
        col.setRemoteRevision(id.changeKey());
        changeCommitted(col);
    }
    else {
        cancelTask(i18n("Failed to update folder"));
    }
}

void EwsResource::collectionRemoved(const Collection &collection)
{
    EwsDeleteFolderRequest *req = new EwsDeleteFolderRequest(mEwsClient, this);
    EwsId::List ids;
    ids.append(EwsId(collection.remoteId(), collection.remoteRevision()));
    req->setFolderIds(ids);
    connect(req, &EwsDeleteFolderRequest::result, this, &EwsResource::folderDeleteRequestFinished);
    req->start();

}

void EwsResource::folderDeleteRequestFinished(KJob *job)
{
    qDebug() << "folderDeleteRequestFinished";
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsDeleteFolderRequest *req = qobject_cast<EwsDeleteFolderRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid EwsDeleteFolderRequest job object"));
        return;
    }

    EwsDeleteFolderRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        changeProcessed();
    }
    else {
        cancelTask(i18n("Failed to delete folder"));
        mFolderSyncState.clear();
        synchronizeCollectionTree();
    }
}

void EwsResource::sendItem(const Akonadi::Item &item)
{
    qDebug() << "sendItem" << item.remoteId();
    EwsItemType type = EwsItemHandler::mimeToItemType(item.mimeType());
    if (type == EwsItemTypeItem) {
        itemSent(item, TransportFailed, QStringLiteral("Item type not supported for creation"));
    }
    else {
        EwsCreateItemJob *job = EwsItemHandler::itemHandler(type)->createItemJob(mEwsClient, item,
            Collection(), this);
        job->setSend(true);
        job->setProperty("item", QVariant::fromValue<Item>(item));
        connect(job, &EwsCreateItemJob::result, this, &EwsResource::itemSendRequestFinished);
        job->start();
    }
}

void EwsResource::itemSendRequestFinished(KJob *job)
{
    Item item = job->property("item").value<Item>();
    if (job->error()) {
        itemSent(item, TransportFailed, job->errorString());
        return;
    }

    EwsCreateItemJob *req = qobject_cast<EwsCreateItemJob*>(job);
    if (!req) {
        itemSent(item, TransportFailed, QStringLiteral("Invalid EwsCreateItemJob job object"));
        return;
    }

    itemSent(item, TransportSucceeded);
}

#ifdef HAVE_SEPARATE_MTA_RESOURCE
void EwsResource::sendMessage(QString id, QByteArray content)
{
    qDebug() << "sendMessage" << id;
    EwsCreateItemRequest *req = new EwsCreateItemRequest(mEwsClient, this);

    EwsItem item;
    item.setType(EwsItemTypeMessage);
    item.setField(EwsItemFieldMimeContent, content);
    req->setItems(EwsItem::List() << item);
    req->setMessageDisposition(EwsDispSendOnly);
    req->setProperty("requestId", id);
    connect(req, &EwsCreateItemRequest::finished, this, &EwsResource::messageSendRequestFinished);
    req->start();
}

void EwsResource::messageSendRequestFinished(KJob *job)
{
    QString id = job->property("requestId").toString();
    if (job->error()) {
        Q_EMIT messageSent(id, job->errorString());
        return;
    }

    EwsCreateItemRequest *req = qobject_cast<EwsCreateItemRequest*>(job);
    if (!req) {
        Q_EMIT messageSent(id, QStringLiteral("Invalid EwsCreateItemRequest job object"));
        return;
    }

    if (req->responses().count() != 1) {
        Q_EMIT messageSent(id, QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    EwsCreateItemRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        Q_EMIT messageSent(id, QString());
    }
    else {
        Q_EMIT messageSent(id, resp.responseMessage());
    }
}
#endif

void EwsResource::foldersModifiedEvent(EwsId::List folders)
{
    Q_FOREACH(const EwsId &id, folders) {
        Collection c;
        c.setRemoteId(id.id());
        CollectionFetchJob *job = new CollectionFetchJob(c, CollectionFetchJob::Base);
        job->setFetchScope(changeRecorder()->collectionFetchScope());
        job->fetchScope().setResource(identifier());
        job->fetchScope().setListFilter(CollectionFetchScope::Sync);
        connect(job, SIGNAL(result(KJob*)), SLOT(foldersModifiedCollectionSyncFinished(KJob*)));
    }

}

void EwsResource::foldersModifiedCollectionSyncFinished(KJob *job)
{
    qCDebugNC(EWSRES_LOG) << job->error() << job->errorText();
    if (job->error()) {
        qCDebug(EWSRES_LOG) << QStringLiteral("Failed to fetch collection tree for sync.");
        return;
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>(job);
    synchronizeCollection(fetchJob->collections()[0].id());
}

void EwsResource::folderTreeModifiedEvent()
{
    synchronizeCollectionTree();
}

void EwsResource::fullSyncRequestedEvent()
{
    synchronize();
}

void EwsResource::clearSyncState()
{
    mSyncState.clear();
}

void EwsResource::clearFolderSyncState(QString folderId)
{
    mSyncState.remove(folderId);
}

void EwsResource::saveState()
{
    QByteArray str;
    QDataStream dataStream(&str, QIODevice::WriteOnly);
    dataStream << mSyncState;
    Settings::self()->setSyncState(qCompress(str, 9).toBase64());
    Settings::self()->save();
}

void EwsResource::doSetOnline(bool online)
{
    if (online) {
        resetUrl();
    } else {
        mSubManager.reset(Q_NULLPTR);
    }
}

bool EwsResource::requestPassword(QString &password, bool ask)
{
    bool status = true;

    if (!mPassword.isEmpty()) {
        password = mPassword;
        return true;
    }

    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                          winIdForDialogs());
    if (wallet && wallet->isOpen()) {
        if (wallet->hasFolder(QStringLiteral("akonadi-ews"))) {
            wallet->setFolder(QStringLiteral("akonadi-ews"));
            wallet->readPassword(config()->name(), password);
        } else {
            wallet->createFolder(QStringLiteral("akonadi-ews"));
        }
    } else {
        status = false;
    }
    delete wallet;

    if (!status) {
        if (!ask) {
            return false;
        }

        KPasswordDialog *dlg = new KPasswordDialog(Q_NULLPTR);
        dlg->setModal(true);
        dlg->setPrompt(i18n("Please enter password for user '%1' and Exchange account '%2'.",
                            Settings::username(), Settings::email()));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        if (dlg->exec() == QDialog::Accepted) {
            password = dlg->password();
            setPassword(password);
        } else {
            return false;
        }
    }

    return true;
}

void EwsResource::setPassword(const QString &password)
{
    mPassword = password;
    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                          winIdForDialogs());
    if (wallet && wallet->isOpen()) {
        if (!wallet->hasFolder(QStringLiteral("akonadi-ews"))) {
            wallet->createFolder(QStringLiteral("akonadi-ews"));
        }
        wallet->setFolder(QStringLiteral("akonadi-ews"));
        wallet->writePassword(config()->name(), password);
    }
    delete wallet;
}

AKONADI_RESOURCE_MAIN(EwsResource)
