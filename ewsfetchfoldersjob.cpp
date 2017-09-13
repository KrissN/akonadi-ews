/*
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
                              const Collection &rootCollection);
    ~EwsFetchFoldersJobPrivate();

    void processRemoteFolders();
    Collection createFolderCollection(const EwsFolder &folder);

    void buildCollectionList();
    void buildChildCollectionList(const Collection &col);
public Q_SLOTS:
    void remoteFolderFullFetchDone(KJob *job);
    void remoteFolderIdFullFetchDone(KJob *job);
    void remoteFolderDetailFetchDone(KJob *job);
public:
    EwsClient &mClient;
    int mPendingFetchJobs;
    int mPendingMoveJobs;
    EwsId::List mRemoteFolderIds;

    const Collection &mRootCollection;

    EwsFolder::List mRemoteChangedFolders;  // Contains details of folders that need update
                                            // (either created or changed)
    QHash<QString, Akonadi::Collection> mCollectionMap;
    QMultiHash<QString, QString> mParentMap;

    EwsFetchFoldersJob *q_ptr;
    Q_DECLARE_PUBLIC(EwsFetchFoldersJob)
};

EwsFetchFoldersJobPrivate::EwsFetchFoldersJobPrivate(EwsFetchFoldersJob *parent, EwsClient &client,
        const Collection &rootCollection)
    : QObject(parent), mClient(client), mRootCollection(rootCollection), q_ptr(parent)
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

    Q_FOREACH (const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
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

        buildCollectionList();

        q->mSyncState = req->syncState();

        q->emitResult();
    } else {
        EwsSyncFolderHierarchyRequest *syncFoldersReq = new EwsSyncFolderHierarchyRequest(mClient, this);
        syncFoldersReq->setFolderId(EwsId(EwsDIdMsgFolderRoot));
        EwsFolderShape shape;
        shape << propPidTagContainerClass;
        shape << EwsPropertyField(QStringLiteral("folder:EffectiveRights"));
        shape << EwsPropertyField(QStringLiteral("folder:ParentFolderId"));
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

    Q_FOREACH (const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
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
        shape << EwsPropertyField(QStringLiteral("folder:EffectiveRights"));
        shape << EwsPropertyField(QStringLiteral("folder:ParentFolderId"));
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

    Q_FOREACH (const EwsGetFolderRequest::Response &resp, req->responses()) {
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

        buildCollectionList();


        q->emitResult();
    }
}

void EwsFetchFoldersJobPrivate::processRemoteFolders()
{
    /* mCollectionMap contains the global collection list keyed by the EWS ID. */
    /* mParentMap contains the parent->child map for each collection. */

    /* Iterate over all changed folders. */
    Q_FOREACH (const EwsFolder &folder, mRemoteChangedFolders) {
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
    }
}

void EwsFetchFoldersJobPrivate::buildCollectionList()
{
    Q_Q(EwsFetchFoldersJob);


    q->mFolders.append(mRootCollection);
    buildChildCollectionList(mRootCollection);

    if (!mCollectionMap.isEmpty()) {
        q->setErrorMsg(QStringLiteral("Found orphaned collections"));
    }
}

void EwsFetchFoldersJobPrivate::buildChildCollectionList(const Collection &col)
{
    Q_Q(EwsFetchFoldersJob);

    QStringList children = mParentMap.values(col.remoteId());
    Q_FOREACH (const QString &childId, children) {
        Collection child(mCollectionMap.take(childId));
        child.setParentCollection(col);
        q->mFolders.append(child);
        buildChildCollectionList(child);
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

EwsFetchFoldersJob::EwsFetchFoldersJob(EwsClient &client, const Akonadi::Collection &rootCollection,
                                       QObject *parent)
    : EwsJob(parent),
      d_ptr(new EwsFetchFoldersJobPrivate(this, client, rootCollection))
{
    qRegisterMetaType<EwsId::List>();
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
    shape << EwsPropertyField(QStringLiteral("folder:EffectiveRights"));
    shape << EwsPropertyField(QStringLiteral("folder:ParentFolderId"));
    syncFoldersReq->setFolderShape(shape);
    if (!mSyncState.isNull()) {
        syncFoldersReq->setSyncState(mSyncState);
    }
    connect(syncFoldersReq, &EwsSyncFolderHierarchyRequest::result, d,
            &EwsFetchFoldersJobPrivate::remoteFolderFullFetchDone);
    // Don't add this as a subjob as the error is handled in its own way rather than throwing an
    // error code to the parent.

    syncFoldersReq->start();
}

