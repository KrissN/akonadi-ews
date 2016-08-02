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

class EwsFetchFoldersJobPrivate : public QObject
{
public:
    EwsFetchFoldersJobPrivate(EwsFetchFoldersJob *parent, EwsClient &client,
                              const QHash<QString, QString> &folderTreeCache,
                              const Collection &rootCollection);
    ~EwsFetchFoldersJobPrivate();

    bool processRemoteFolders();
    Collection createFolderCollection(const EwsFolder &folder);

    void buildCollectionList(const Collection::List &unknownCollections);
    void buildChildCollectionList(QHash<QString, Collection> unknownColHash, const Collection &col);
public Q_SLOTS:
    void remoteFolderFullFetchDone(KJob *job);
    void remoteFolderIncrFetchDone(KJob *job);
    void remoteFolderIdFullFetchDone(KJob *job);
    void remoteFolderDetailFetchDone(KJob *job);
    void unknownParentCollectionsFetchDone(KJob *job);
    void localFolderMoveDone(KJob *job);
public:
    EwsClient& mClient;
    int mPendingFetchJobs;
    int mPendingMoveJobs;
    EwsId::List mRemoteFolderIds;

    const Collection &mRootCollection;

    EwsFolder::List mRemoteChangedFolders;  // Contains details of folders that need update
                                            // (either created or changed)
    QStringList mRemoteDeletedIds;  // Contains IDs of folders that have been deleted.
    QStringList mRemoteChangedIds;  // Contains IDs of folders that have changed (not created).

    //QHash<QString, Akonadi::Collection> mDummyMap;
    //QMultiHash<QString, Akonadi::Collection> mReparentMap;
    const QHash<QString, QString> &mFolderTreeCache;
    QList<QString> mUnknownParentList;

    QHash<QString, Akonadi::Collection> mCollectionMap;
    QMultiHash<QString, QString> mParentMap;

    EwsFetchFoldersJob *q_ptr;
    Q_DECLARE_PUBLIC(EwsFetchFoldersJob)
};

EwsFetchFoldersJobPrivate::EwsFetchFoldersJobPrivate(EwsFetchFoldersJob *parent, EwsClient &client,
                                                     const QHash<QString, QString> &folderTreeCache,
                                                     const Collection &rootCollection)
    : QObject(parent), mClient(client), mRootCollection(rootCollection),
      mFolderTreeCache(folderTreeCache), q_ptr(parent)
{
    mPendingFetchJobs = 0;
    mPendingMoveJobs = 0;
}

EwsFetchFoldersJobPrivate::~EwsFetchFoldersJobPrivate()
{
}

void EwsFetchFoldersJobPrivate::remoteFolderFullFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object");
        q->setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        q->emitResult();
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
                &EwsFetchFoldersJobPrivate::remoteFolderIdFullFetchDone);
        q->addSubjob(syncFoldersReq);
        syncFoldersReq->start();

        return;
    }

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        if (ch.type() == EwsSyncFolderHierarchyRequest::Create) {
            mRemoteChangedFolders.append(ch.folder());
        } else {
            q->setErrorMsg(QStringLiteral("Got non-create change for full sync."));
            q->emitResult();
            return;
        }
    }

    if (req->includesLastItem()) {
        processRemoteFolders();

        Collection::List colList;
        colList.append(mRootCollection);
        buildCollectionList(colList);

        q->mFolders = q->mChangedFolders;
        q->mSyncState = req->syncState();

        q->emitResult();
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
                &EwsFetchFoldersJobPrivate::remoteFolderFullFetchDone);
        syncFoldersReq->start();
    }
}

void EwsFetchFoldersJobPrivate::remoteFolderIdFullFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object");
        q->setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        q->emitResult();
        return;
    }

    if (req->error()) {
        return;
    }

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        if (ch.type() == EwsSyncFolderHierarchyRequest::Create) {
            mRemoteFolderIds.append(ch.folder()[EwsFolderFieldFolderId].value<EwsId>());
        } else {
            q->setErrorMsg(QStringLiteral("Got non-create change for full sync."));
            q->emitResult();
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
                    &EwsFetchFoldersJobPrivate::remoteFolderDetailFetchDone);
            req->start();
            q->addSubjob(req);
            mPendingFetchJobs++;
        }

        qCDebugNC(EWSRES_LOG) << QStringLiteral("Starting %1 folder fetch jobs.").arg(mPendingFetchJobs);

        q->mSyncState = req->syncState();
    } else {
        EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
        syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
        EwsFolderShape shape(EwsShapeIdOnly);
        syncFoldersReq->setFolderShape(shape);
        syncFoldersReq->setSyncState(req->syncState());
        connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, this,
                &EwsFetchFoldersJobPrivate::remoteFolderIdFullFetchDone);
        q->addSubjob(syncFoldersReq);
        syncFoldersReq->start();
    }
}

void EwsFetchFoldersJobPrivate::remoteFolderDetailFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    EwsGetFolderRequest *req = qobject_cast<EwsGetFolderRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsGetFolderRequest job object");
        q->setErrorMsg(QStringLiteral("Invalid EwsGetFolderRequest job object"));
        q->emitResult();
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

        q->mFolders = q->mChangedFolders;

        q->emitResult();
   }
}

void EwsFetchFoldersJobPrivate::remoteFolderIncrFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    EwsSyncFolderHierarchyRequest *req = qobject_cast<EwsSyncFolderHierarchyRequest*>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsSyncFolderHierarchyRequestjob object");
        q->setErrorMsg(QStringLiteral("Invalid EwsSyncFolderHierarchyRequest job object"));
        q->emitResult();
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

    q->mSyncState = req->syncState();

    if (!processRemoteFolders()) {
        buildCollectionList(Collection::List());

        q->setErrorMsg(QStringLiteral("No parent collections to fetch found for incremental sync."));
        q->emitResult();
    }
}

bool EwsFetchFoldersJobPrivate::processRemoteFolders()
{
    Q_Q(const EwsFetchFoldersJob);

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
    if (q->mFullSync) {
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
                &EwsFetchFoldersJobPrivate::unknownParentCollectionsFetchDone);

        return true;
    } else {
        /* No unknown parent collections. */
        return false;
    }


}

void EwsFetchFoldersJobPrivate::unknownParentCollectionsFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    if (job->error()) {
        q->setErrorMsg(QStringLiteral("Failed to fetch unknown parent collections."));
        q->emitResult();
        return;
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>(job);
    Q_ASSERT(fetchJob);

    buildCollectionList(fetchJob->collections());

    if (!mPendingMoveJobs) {
        q->emitResult();
    }
}

void EwsFetchFoldersJobPrivate::buildCollectionList(const Collection::List &unknownCollections)
{
    Q_Q(EwsFetchFoldersJob);

    QHash<QString, Collection> unknownColHash;
    Q_FOREACH(const Collection &col, unknownCollections) {
        unknownColHash.insert(col.remoteId(), col);
    }
    Q_FOREACH(const Collection &col, unknownCollections) {
        if (mRemoteDeletedIds.contains(col.remoteId())) {
            /* This collection has been removed. */
            q->mDeletedFolders.append(col);
        } else if (q->mFullSync || mUnknownParentList.contains(col.remoteId())) {
            /* This collection is parent to a subtree of collections that have been added or changed.
             * Build a list of collections to update. */
            q->mChangedFolders.append(col);
            buildChildCollectionList(unknownColHash, col);
        }
    }

    if (!mCollectionMap.isEmpty()) {
        q->setErrorMsg(QStringLiteral("Found orphaned collections"));
    }
}

void EwsFetchFoldersJobPrivate::buildChildCollectionList(QHash<QString, Collection> unknownColHash,
                                                         const Collection &col)
{
    Q_Q(EwsFetchFoldersJob);

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
            connect(job, &CollectionMoveJob::result, this, &EwsFetchFoldersJobPrivate::localFolderMoveDone);
            mPendingMoveJobs++;
            job->start();
        }
        q->mChangedFolders.append(child);
        buildChildCollectionList(unknownColHash, child);
    }
}

void EwsFetchFoldersJobPrivate::localFolderMoveDone(KJob *job)
{
    Q_Q(EwsFetchFoldersJob);

    if (job->error()) {
        q->setErrorMsg(QStringLiteral("Failed to move collection."));
        q->emitResult();
        return;
    }

    if (--mPendingMoveJobs == 0) {
        q->emitResult();
    }
}


Collection EwsFetchFoldersJobPrivate::createFolderCollection(const EwsFolder &folder)
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

EwsFetchFoldersJob::EwsFetchFoldersJob(EwsClient &client, const QString &syncState,
                                       const Akonadi::Collection &rootCollection,
                                       const QHash<QString, QString> &folderTreeCache, QObject *parent)
    : EwsJob(parent), mSyncState(syncState),
      d_ptr(new EwsFetchFoldersJobPrivate(this, client, folderTreeCache, rootCollection))
{
    Q_D(EwsFetchFoldersJob);

    qRegisterMetaType<EwsId::List>();

    mFullSync = syncState.isNull();
}

EwsFetchFoldersJob::~EwsFetchFoldersJob()
{
}

void EwsFetchFoldersJob::start()
{
    Q_D(const EwsFetchFoldersJob);

    EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(d->mClient, this);
    syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
    EwsFolderShape shape;
    shape << propPidTagContainerClass;
    shape << EwsPropertyField("folder:EffectiveRights");
    shape << EwsPropertyField("folder:ParentFolderId");
    syncFoldersReq->setFolderShape(shape);
    if (!mSyncState.isNull()) {
        syncFoldersReq->setSyncState(mSyncState);
    }
    connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, d,
            mFullSync ? &EwsFetchFoldersJobPrivate::remoteFolderFullFetchDone : &EwsFetchFoldersJobPrivate::remoteFolderIncrFetchDone);
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

