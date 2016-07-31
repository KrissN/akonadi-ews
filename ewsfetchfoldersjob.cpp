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

#include "ewsfetchfoldersjob.h"

#include <KMime/Message>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>
#include <AkonadiCore/CollectionStatistics>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionMoveJob>

#include "ewssyncfolderhierarchyrequest.h"
#include "ewsgetfolderrequest.h"
#include "ewseffectiverights.h"
#include "ewsclient.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidTagContainerClass(0x3613, EwsPropTypeString);

static Q_CONSTEXPR int fetchBatchSize = 50;

EwsFetchFoldersJob::EwsFetchFoldersJob(EwsClient &client, const QString &syncState,
                                       const Akonadi::Collection &rootCollection,
                                       const QHash<QString, QString> &folderTreeCache, QObject *parent)
    : EwsJob(parent), mClient(client), mRootCollection(rootCollection), mSyncState(syncState),
      mFullSync(syncState.isNull()), mPendingFetchJobs(0), mPendingMoveJobs(0),
      mFolderTreeCache(folderTreeCache)
{
    qRegisterMetaType<EwsId::List>();
}

EwsFetchFoldersJob::~EwsFetchFoldersJob()
{
}

void EwsFetchFoldersJob::start()
{
    EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
    syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
    EwsFolderShape shape;
    shape << propPidTagContainerClass;
    shape << EwsPropertyField("folder:EffectiveRights");
    shape << EwsPropertyField("folder:ParentFolderId");
    syncFoldersReq->setFolderShape(shape);
    if (!mSyncState.isNull()) {
        syncFoldersReq->setSyncState(mSyncState);
    }
    connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, this,
            mFullSync ? &EwsFetchFoldersJob::remoteFolderFullFetchDone : &EwsFetchFoldersJob::remoteFolderIncrFetchDone);
    // Don't add this as a subjob as the error is handled in its own way rather than throwing an
    // error code to the parent.

    syncFoldersReq->start();
}

void EwsFetchFoldersJob::localFolderFetchDone(KJob *job)
{

}

void EwsFetchFoldersJob::localCollectionsRetrieved(const Collection::List &collections)
{

}

void EwsFetchFoldersJob::remoteFolderFullFetchDone(KJob *job)
{
    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object");
        setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        emitResult();
        return;
    }

    if (req->error()) {
        /* It has been reported that the SyncFolderHierarchyRequest can fail with Internal Server
         * Error (possibly because of a large number of folders). In order to work around this
         * try to fallback to fetching just the folder identifiers and retrieve the details later. */
        qCDebug(EWSRES_LOG) << QStringLiteral("Full fetch failed. Trying to fetch ids only.");

        EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
        syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
        EwsFolderShape shape(EwsShapeIdOnly);
        syncFoldersReq->setFolderShape(shape);
        connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, this,
                &EwsFetchFoldersJob::remoteFolderIdFullFetchDone);
        addSubjob(syncFoldersReq);
        syncFoldersReq->start();

        return;
    }

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        if (ch.type() == EwsSyncFolderHierarchyRequest::Create) {
            mRemoteChangedFolders.append(ch.folder());
        } else {
            setErrorMsg(QStringLiteral("Got non-create change for full sync."));
            emitResult();
            return;
        }
    }

    if (req->includesLastItem()) {
        processRemoteFolders();

        Collection::List colList;
        colList.append(mRootCollection);
        buildCollectionList(colList);

        mFolders = mChangedFolders;
        mSyncState = req->syncState();

        emitResult();
    } else {
        EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
        syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
        EwsFolderShape shape;
        shape << propPidTagContainerClass;
        shape << EwsPropertyField("folder:EffectiveRights");
        shape << EwsPropertyField("folder:ParentFolderId");
        syncFoldersReq->setFolderShape(shape);
        syncFoldersReq->setSyncState(req->syncState());
        connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, this,
                &EwsFetchFoldersJob::remoteFolderFullFetchDone);
        syncFoldersReq->start();
    }
}

void EwsFetchFoldersJob::remoteFolderIdFullFetchDone(KJob *job)
{
    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object");
        setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        emitResult();
        return;
    }

    if (req->error()) {
        return;
    }

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        if (ch.type() == EwsSyncFolderHierarchyRequest::Create) {
            mRemoteFolderIds.append(ch.folder()[EwsFolderFieldFolderId].value<EwsId>());
        } else {
            setErrorMsg(QStringLiteral("Got non-create change for full sync."));
            emitResult();
            return;
        }
    }

    if (req->includesLastItem()) {
        EwsFolderShape shape(EwsShapeDefault);
        shape << propPidTagContainerClass;
        shape << EwsPropertyField("folder:EffectiveRights");
        shape << EwsPropertyField("folder:ParentFolderId");
        mPendingFetchJobs = 0;

        for (int i = 0; i < mRemoteFolderIds.size(); i += fetchBatchSize) {
            EwsGetFolderRequest *req = new EwsGetFolderRequest(mClient, this);
            req->setFolderIds(mRemoteFolderIds.mid(i, fetchBatchSize));
            req->setFolderShape(shape);
            connect(req, &EwsSyncFolderHierarchyRequest::result, this,
                    &EwsFetchFoldersJob::remoteFolderDetailFetchDone);
            req->start();
            addSubjob(req);
            mPendingFetchJobs++;
        }

        qCDebugNC(EWSRES_LOG) << QStringLiteral("Starting %1 folder fetch jobs.").arg(mPendingFetchJobs);

        mSyncState = req->syncState();
    } else {
        EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
        syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
        EwsFolderShape shape(EwsShapeIdOnly);
        syncFoldersReq->setFolderShape(shape);
        syncFoldersReq->setSyncState(req->syncState());
        connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, this,
                &EwsFetchFoldersJob::remoteFolderIdFullFetchDone);
        addSubjob(syncFoldersReq);
        syncFoldersReq->start();
    }
}

void EwsFetchFoldersJob::remoteFolderDetailFetchDone(KJob *job)
{
    EwsGetFolderRequest *req = qobject_cast<EwsGetFolderRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsGetFolderRequest job object");
        setErrorMsg(QStringLiteral("Invalid EwsGetFolderRequest job object"));
        emitResult();
        return;
    }

    if (req->error()) {
        return;
    }

    Q_FOREACH(const EwsGetFolderRequest::Response& resp, req->responses()) {
        if (resp.isSuccess()) {
            mRemoteChangedFolders.append(resp.folder());
        } else {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch folder details.");
        }
    }

    mPendingFetchJobs--;
    qCDebugNC(EWSRES_LOG) << QStringLiteral("%1 folder fetch jobs pending").arg(mPendingFetchJobs);

    if (mPendingFetchJobs == 0) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("All folder fetch jobs complete");

        processRemoteFolders();

        Collection::List colList;
        colList.append(mRootCollection);
        buildCollectionList(colList);

        mFolders = mChangedFolders;

        emitResult();
   }
}

void EwsFetchFoldersJob::remoteFolderIncrFetchDone(KJob *job)
{
    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequestjob object");
        setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        emitResult();
        return;
    }

    if (req->error()) {
        return;
    }

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        switch (ch.type()) {
        case EwsSyncFolderHierarchyRequest::Update:
        {
            EwsId id = ch.folder()[EwsFolderFieldFolderId].value<EwsId>();
            mRemoteChangedIds.append(id.id());
        }
        /* no break */
        case EwsSyncFolderHierarchyRequest::Create:
        {
            mRemoteChangedFolders.append(ch.folder());
            break;
        }
        case EwsSyncFolderHierarchyRequest::Delete:
        {
            mRemoteDeletedIds.append(ch.folderId().id());
            break;
        }
        default:
            break;
        }
    }

    mSyncState = req->syncState();

    if (!processRemoteFolders()) {
        buildCollectionList(Collection::List());

        setErrorMsg(QStringLiteral("No parent collections to fetch found for incremental sync."));
        emitResult();
    }
}

bool EwsFetchFoldersJob::processRemoteFolders()
{
    /* mCollectionMap contains the global collection list keyed by the EWS ID. */
    /* mParentMap contains the parent->child map for each collection. */
    /* mUnknownParentList contains the list of parent collections that are not known and
     *     will need to be fetched in order to provide real parents for child collections that
     *     might have changed or been added. */


    /* Iterate over all changed folders. */
    Q_FOREACH(const EwsFolder &folder, mRemoteChangedFolders) {
        /* Create a collection for each folder. */
        Collection c = createFolderCollection(folder);

        /* Insert it into the global collection list. */
        mCollectionMap.insert(c.remoteId(), c);

        /* Determine the parent and insert a parent->child relationship.
         * Don't use Collection::setParentCollection() yet as the collection object will be updated
         * which will cause the parent->child relationship to be broken. This happens because the
         * collection object holds a copy of the parent collection object. An update to that
         * object in the list will not be visible in the copy inside of the child object. */
        EwsId parentId = folder[EwsFolderFieldParentFolderId].value<EwsId>();
        mParentMap.insert(parentId.id(), c.remoteId());

        /* Check if the parent of this collection is known. If not mark the parent as unknown. */
        if (!mCollectionMap.contains(parentId.id()) && !mUnknownParentList.contains(parentId.id())) {
            mUnknownParentList.append(parentId.id());
        }

        /* Remove this collection from the unknown parent list. */
        mUnknownParentList.removeOne(c.remoteId());
    }

    /* The unknown parent list may contain the root folder. Normally this is not a problem as all
     * of these folders will be fetched from Akonadi in the next step. However when doing a full
     * sync it may happen that this is actually an initial sync and the root collection doesn't
     * exist yet. Remove it from the unknown list. It will be used later as the root for building
     * the collection tree. */
    if (mFullSync) {
        mUnknownParentList.removeOne(mRootCollection.remoteId());

        /* In case if a full sync the only unknown parent must be the root. If any additional
         * parents are unknown then something is wrong. */
        if (!mUnknownParentList.isEmpty()) {
            qCWarningNC(EWSRES_LOG) << "Unknown parents found for full sync.";
        }
        return false;
    } else if (!mUnknownParentList.isEmpty() || !mRemoteDeletedIds.isEmpty()) {
        /* Fetch any unknown parent collections from Akonadi. */
        Collection::List fetchList;
        Q_FOREACH(const QString &id, mUnknownParentList + mRemoteDeletedIds) {
            Collection c;
            c.setRemoteId(id);
            fetchList.append(c);
        }
        Q_FOREACH(const QString &id, mRemoteChangedIds) {
            if (!mUnknownParentList.contains(id)) {
                Collection c;
                c.setRemoteId(id);
                fetchList.append(c);
            } else {
                /* Remotely changed folders should not appear here as the code would have put their
                 * parents into the unknown list instead. */
                qCWarningNC(EWSRES_LOG) << QStringLiteral("Found remotely changed folder %1 on unknown folder list")
                                .arg(id);
            }
        }
        CollectionFetchJob *fetchJob = new CollectionFetchJob(fetchList, CollectionFetchJob::Base);
        CollectionFetchScope scope;
        scope.setAncestorRetrieval(CollectionFetchScope::All);
        fetchJob->setFetchScope(scope);
        connect(fetchJob, &CollectionFetchJob::result, this,
                &EwsFetchFoldersJob::unknownParentCollectionsFetchDone);

        return true;
    } else {
        /* No unknown parent collections. */
        return false;
    }


}

void EwsFetchFoldersJob::unknownParentCollectionsFetchDone(KJob *job)
{
    if (job->error()) {
        setErrorMsg(QStringLiteral("Failed to fetch unknown parent collections."));
        emitResult();
        return;
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>(job);
    Q_ASSERT(fetchJob);

    buildCollectionList(fetchJob->collections());

    if (!mPendingMoveJobs) {
        emitResult();
    }
}

void EwsFetchFoldersJob::buildCollectionList(const Akonadi::Collection::List &unknownCollections)
{
    QHash<QString, Collection> unknownColHash;
    Q_FOREACH(const Collection &col, unknownCollections) {
        unknownColHash.insert(col.remoteId(), col);
    }
    Q_FOREACH(const Collection &col, unknownCollections) {
        if (mRemoteDeletedIds.contains(col.remoteId())) {
            /* This collection has been removed. */
            mDeletedFolders.append(col);
        } else if (mFullSync || mUnknownParentList.contains(col.remoteId())) {
            /* This collection is parent to a subtree of collections that have been added or changed.
             * Build a list of collections to update. */
            mChangedFolders.append(col);
            buildChildCollectionList(unknownColHash, col);
        }
    }

    if (!mCollectionMap.isEmpty()) {
        setErrorMsg(QStringLiteral("Found orphaned collections"));
    }
}

void EwsFetchFoldersJob::buildChildCollectionList(QHash<QString, Collection> unknownColHash,
                                                  const Akonadi::Collection &col)
{
    QStringList children = mParentMap.values(col.remoteId());
    Q_FOREACH(const QString &childId, children) {
        Collection child(mCollectionMap.take(childId));
        child.setParentCollection(col);
        if (mRemoteChangedIds.contains(childId) &&
            unknownColHash.value(childId).parentCollection().remoteId() != col.remoteId()) {
            /* This collection has changed parents. Since Akonadi cannot detect collection moves
             * it is necessary to manually issue a request.
             */
            CollectionMoveJob *job = new CollectionMoveJob(unknownColHash.value(childId), col);
            connect(job, &CollectionMoveJob::result, this, &EwsFetchFoldersJob::localFolderMoveDone);
            mPendingMoveJobs++;
            job->start();
        }
        mChangedFolders.append(child);
        buildChildCollectionList(unknownColHash, child);
    }
}

void EwsFetchFoldersJob::localFolderMoveDone(KJob *job)
{
    if (job->error()) {
        setErrorMsg(QStringLiteral("Failed to move collection."));
        emitResult();
        return;
    }

    if (--mPendingMoveJobs == 0) {
        emitResult();
    }
}


Collection EwsFetchFoldersJob::createFolderCollection(const EwsFolder &folder)
{
    Collection collection;
    collection.setName(folder[EwsFolderFieldDisplayName].toString());
    QStringList mimeTypes;
    QString contClass = folder[propPidTagContainerClass].toString();
    mimeTypes.append(Collection::mimeType());
    switch (folder.type()) {
    case EwsFolderTypeCalendar:
        mimeTypes.append(KCalCore::Event::eventMimeType());
        break;
    case EwsFolderTypeContacts:
        mimeTypes.append(KContacts::Addressee::mimeType());
        mimeTypes.append(KContacts::ContactGroup::mimeType());
        break;
    case EwsFolderTypeTasks:
        mimeTypes.append(KCalCore::Todo::todoMimeType());
        break;
    case EwsFolderTypeMail:
        if (contClass == QStringLiteral("IPF.Note") || contClass.isEmpty()) {
            mimeTypes.append(KMime::Message::mimeType());
        }
        break;
    default:
        break;
    }
    collection.setContentMimeTypes(mimeTypes);
    Collection::Rights colRights;
    EwsEffectiveRights ewsRights = folder[EwsFolderFieldEffectiveRights].value<EwsEffectiveRights>();
    // FIXME: For now full read/write support is only implemented for e-mail. In order to avoid
    // potential problems block write access to all other folder types.
    if (folder.type() == EwsFolderTypeMail) {
        if (ewsRights.canDelete()) {
            colRights |= Collection::CanDeleteCollection | Collection::CanDeleteItem;
        }
        if (ewsRights.canModify()) {
            colRights |= Collection::CanChangeCollection | Collection::CanChangeItem;
        }
        if (ewsRights.canCreateContents()) {
            colRights |= Collection::CanCreateItem;
        }
        if (ewsRights.canCreateHierarchy()) {
            colRights |= Collection::CanCreateCollection;
        }
    }
    collection.setRights(colRights);
    EwsId id = folder[EwsFolderFieldFolderId].value<EwsId>();
    collection.setRemoteId(id.id());
    collection.setRemoteRevision(id.changeKey());

    return collection;
}
