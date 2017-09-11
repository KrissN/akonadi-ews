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

#include "ewsfetchfoldersincrjob.h"

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

/*
 * Performing an incremental folder tree update relies on the SyncFolderHierarchy EWS request,
 * which returns a list of changes to the folder tree. Each of the change can be a folder creation,
 * modification or removal notification.
 *
 * The EwsFetchFoldersIncrJob class starts by executing the SyncFolderHierarchy request in order to
 * retrieve the remote changes.
 *
 * Once that is completed the remoteFolderIncrFetchDone() method processes the changes. For each
 * one a folder descriptor (FolderDescr) is created and inserted into the folder hash (mFolderHash)
 * keyed by the folder EWS identifier. The folder hash contains all collections that are being
 * processed during the update. The flags member is used to determine the type of each collection.
 * For each change the corresponding local collection needs to be retrieved in order to correctly
 * pass the change list to Akonadi. The following rules apply:
 *
 *  * For created folders the parent collection is retrieved. This is necessary to put a valid parent
 *    collection to the newly created one. In order to handle cascaded folder creations
 *    (i.e. two folders are created, one child of the other) the parent collection is only retrieved
 *    for the topmost created folder.
 *  * For updated/modified folders both the current (corresponding to the EWS updated folder) and
 *    parent collections are retrieved. The current collection is retrieved to update only the
 *    changed information in the collection. The parent collection is retrieved in order to detect
 *    and handle collection moves as in such case the Akonadi-side parent will not be the same as
 *    the EWS-side parent (the parent retrieved is the EWS-side parent and the Akonadi-side parent
 *    will be known as part of the current collection once retrieved from Akonadi).
 *  * For deleted folders the current (corresponding to the EWS deleted folder) is retrieved.
 *
 * After the local Akonadi collections are retrieved the objects are put into their corresponding
 * folder descriptors in the folder hash.
 *
 * Having information about all remote changed folders and their local Akonadi collections the main
 * part of the synchronization process can be started.
 *
 * In the first pass the processRemoteFolders() method looks at all folders in the hash table.
 * Setting the parent-child relationships is performed at this stage only when the relevant parent
 * collection has already been processed (the FolderDescr::Processed flag is set). This ensures
 * that the parent-child relationships are set in the down-the-tree order. In case this condition
 * is not met for a collection the need for an extra reparenting pass is flagged and the parent
 * collection setting is not performed.
 *
 * Two types of folders are of main interest:
 *
 *  * For created folders the Akonadi collection object is created and populated with data obtained
 *    from Exchange. If the parent collection has alredy been processed in this pass the parent is
 *    set on the newly created collection.
 *  * For modified folders the Akonadi collection object that was retrieved previously is updated
 *    with data obtained from Exchange. If the folder was moved (the Akonadi parent differs from the
 *    Exchange parent) a collection move is attempted. This needs to be done explicitly using a
 *    CollectionMoveJob as Akonadi is unable to detect collection moves in the sync code. Similar
 *    to the created folder case the move is only performed in case the new parent is flagged as
 *    processed. Additionally the code checks if the new parent is a newly created folder. In such
 *    case the whole incremental sync is aborted as handling this rare corner case would introduce
 *    extra complexity. In case of incremental sync failure the resource will fallback to a full
 *    sync that will handle the case.
 *
 * Regardless of collection type the first pass also builds a list of top-level collections
 * (i.e. ones for which the parent is not in the folder hash) and a hash containing the parent-child
 * relationship. Both lists will be needed in case a reparenting pass is needed.
 *
 * The optional reparenting pass follows the first pass. It is performed if processing of at least
 * one collection failed due to an unprocessed parent. The reparenting pass focuses on the top-level
 * folders and starting from each recursively goes into its children setting their parent to itself.
 * The pass also processes any delayed collection moves in case executing them was impossible in the
 * first pass.
 *
 * The final stage of the synchronization process builds a list of changed and deleted collections
 * for Akonadi. At this stage all collections must be processed, otherwise an error is raised. If
 * no collection moves have been executed the job is completed. Otherwise the completion is
 * singalled once all moves are done.
 */

static const EwsPropertyField propPidTagContainerClass(0x3613, EwsPropTypeString);

class FolderDescr {
public:
    typedef enum {
        RemoteCreated = 0x0001,
        RemoteUpdated = 0x0002,
        RemoteDeleted = 0x0004,
        Processed = 0x0008
    } Flag;
    Q_DECLARE_FLAGS(Flags, Flag)

    FolderDescr() {};

    Akonadi::Collection collection;
    Flags flags;
    EwsFolder ewsFolder;

    bool isCreated() const { return flags & RemoteCreated; };
    bool isModified() const { return flags & RemoteUpdated; };
    bool isRemoved() const { return flags & RemoteDeleted; };
    bool isProcessed() const { return flags & Processed; };
    QString parent() const { return ewsFolder.isValid() ? ewsFolder[EwsFolderFieldParentFolderId].value<EwsId>().id() : QString(); };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FolderDescr::Flags)

class EwsFetchFoldersIncrJobPrivate : public QObject
{
public:
    EwsFetchFoldersIncrJobPrivate(EwsFetchFoldersIncrJob *parent, EwsClient &client,
                                  const Collection &rootCollection);
    ~EwsFetchFoldersIncrJobPrivate();

    bool processRemoteFolders();
    void updateFolderCollection(Collection &collection, const EwsFolder &folder);

    void reparentRemoteFolder(const QString &id);
    void moveCollection(const FolderDescr &fd);
public Q_SLOTS:
    void remoteFolderIncrFetchDone(KJob *job);
    void localFolderFetchDone(KJob *job);
    void localFolderMoveDone(KJob *job);
public:
    EwsClient& mClient;
    int mPendingMoveJobs;
    EwsId::List mRemoteFolderIds;

    const Collection &mRootCollection;

    QMultiHash<QString, QString> mParentMap;

    QHash<QString, FolderDescr> mFolderHash;

    EwsFetchFoldersIncrJob *q_ptr;
    Q_DECLARE_PUBLIC(EwsFetchFoldersIncrJob)
};

EwsFetchFoldersIncrJobPrivate::EwsFetchFoldersIncrJobPrivate(EwsFetchFoldersIncrJob *parent, EwsClient &client,
                                                             const Collection &rootCollection)
    : QObject(parent), mClient(client), mRootCollection(rootCollection), q_ptr(parent)
{
    mPendingMoveJobs = 0;
}

EwsFetchFoldersIncrJobPrivate::~EwsFetchFoldersIncrJobPrivate()
{
}

void EwsFetchFoldersIncrJobPrivate::remoteFolderIncrFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersIncrJob);

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

    if (req->changes().isEmpty()) {
        /* Nothing to do. */
        q->emitResult();
        return;
    }

    /* Build a list of local collections to fetch in response to the remote changes.
     * Use a hash to auto-eliminate duplicates. */
    QHash<QString, Collection> localFetchHash;

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        FolderDescr fd;
        Collection c;

        switch (ch.type()) {
        case EwsSyncFolderHierarchyRequest::Update:
        {
            fd.ewsFolder = ch.folder();
            fd.flags |= FolderDescr::RemoteUpdated;
            EwsId id = fd.ewsFolder[EwsFolderFieldFolderId].value<EwsId>();
            mFolderHash.insert(id.id(), fd);

            /* For updated folders fetch the collection corresponding to that folder and its parent
             * (the parent will be needed in case of a collection move) */
            Collection c2;
            c2.setRemoteId(fd.parent());
            localFetchHash.insert(c2.remoteId(), c2);

            c.setRemoteId(id.id());
            localFetchHash.insert(c.remoteId(), c);
            break;
        }
        case EwsSyncFolderHierarchyRequest::Create:
        {
            fd.ewsFolder = ch.folder();
            fd.flags |= FolderDescr::RemoteCreated;
            EwsId id = fd.ewsFolder[EwsFolderFieldFolderId].value<EwsId>();
            mFolderHash.insert(id.id(), fd);

            c.setRemoteId(fd.parent());
            /* For created folders fetch the parent collection on Exchange side. Don't do this
             * when the parent collection has also been created as it would fail. */
            if (!mFolderHash.value(fd.parent()).isCreated()) {
                localFetchHash.insert(c.remoteId(), c);
            }
            break;
        }
        case EwsSyncFolderHierarchyRequest::Delete:
        {
            fd.flags |= FolderDescr::RemoteDeleted;
            mFolderHash.insert(ch.folderId().id(), fd);

            /* For deleted folders fetch the collection corresponding to the deleted folder. */
            c.setRemoteId(ch.folderId().id());
            localFetchHash.insert(c.remoteId(), c);
            break;
        }
        default:
            break;
        }
    }

    if (localFetchHash.isEmpty()) {
        /* In either case at least one folder is expected to be queued for fetching. */
        q->setErrorMsg(QStringLiteral("Expected at least one local folder to fetch."));
        q->emitResult();
        return;
    }

    q->mSyncState = req->syncState();

    CollectionFetchJob *fetchJob = new CollectionFetchJob(localFetchHash.values().toVector(),
        CollectionFetchJob::Base);
    CollectionFetchScope scope;
    scope.setAncestorRetrieval(CollectionFetchScope::All);
    fetchJob->setFetchScope(scope);
    connect(fetchJob, &CollectionFetchJob::result, this,
            &EwsFetchFoldersIncrJobPrivate::localFolderFetchDone);
    q->addSubjob(fetchJob);
}

void EwsFetchFoldersIncrJobPrivate::localFolderFetchDone(KJob *job)
{
    Q_Q(EwsFetchFoldersIncrJob);

    if (job->error()) {
        q->setErrorMsg(QStringLiteral("Failed to fetch local collections."));
        q->emitResult();
        return;
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>(job);
    Q_ASSERT(fetchJob);

    Q_FOREACH(const Collection &col, fetchJob->collections()) {
        /* Retrieve the folder descriptor for this collection. Note that a new descriptor will be
         * created if it does not yet exist. */
        FolderDescr &fd = mFolderHash[col.remoteId()];
        fd.collection = col;
        if (!fd.flags) {
            /* This collection has just been created and this means that it's a parent collection
             * added in response to a created folder. Since the collection is here just for reference
             * it will not be processed by processRemoteFolders() and can be marked accordingly. */
            fd.flags |= FolderDescr::Processed;
        }
    }

    if (!processRemoteFolders()) {
        q->setErrorMsg(QStringLiteral("Failed to process remote folder list."));
        q->emitResult();
    }

    if (!mPendingMoveJobs) {
        q->emitResult();
    }
    /* Otherwise wait for the move requests to finish. */
}

bool EwsFetchFoldersIncrJobPrivate::processRemoteFolders()
{
    Q_Q(EwsFetchFoldersIncrJob);

    /* The list of top-level collections. It contains identifiers of collections for which the
     * parent collection is not in the folder hash. This list is used at a later stage when
     * setting collections parents. Building a top-level list is necessary as those updates can
     * only be safely performed down the tree. */
    QStringList topLevelList;

    bool reparentPassNeeded = false;

    /* Iterate over all changed folders. */
    for (auto it = mFolderHash.begin(); it != mFolderHash.end(); ++it) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Processing: ") << it.key();

        if (it->isModified()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Collection was modified");
            updateFolderCollection(it->collection, it->ewsFolder);

            if (it->parent() != it->collection.parentCollection().remoteId()) {
                /* This collection has been moved. Since Akonadi currently cannot handle collection
                 * moves the resource needs to manually move it. */
                qCDebugNC(EWSRES_LOG) << QStringLiteral("Collection was moved");

                /* Before moving check if the parent exists and has been processed. */
                auto parentIt = mFolderHash.find(it->parent());
                if (parentIt == mFolderHash.end()) {
                    q->setErrorMsg(QStringLiteral("Found moved collection without new parent."));
                    return false;
                }

                if (parentIt->isCreated()) {
                    /* Further workarounds could be done here to ensure that the parent is manually
                     * created before triggering a move but this would just unnecessarily complicate
                     * matters. Instead just surrender and retry with a full sync. */
                    q->setErrorMsg(QStringLiteral("Found moved collection to a just created parent."));
                    return false;
                }

                if (!parentIt->isProcessed()) {
                    qCDebugNC(EWSRES_LOG) << QStringLiteral("Parent not yet processed - delaying");
                    /* The new parent collection is not yet processed - defer the move to make
                     * sure all the oprtations are done in down-the-tree order. */
                    reparentPassNeeded = true;
                } else {
                    moveCollection(*it);
                    it->collection.setParentCollection(parentIt->collection);
                    it->flags |= FolderDescr::Processed;
                }
            } else {
                /* No collection move happening so nothing else to for this one. */
                it->flags |= FolderDescr::Processed;
            }
        } else if (it->isCreated()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Collection was created");
            it->collection.setRemoteId(it.key());
            updateFolderCollection(it->collection, it->ewsFolder);

            auto parentIt = mFolderHash.find(it->parent());
            if (parentIt == mFolderHash.end()) {
                q->setErrorMsg(QStringLiteral("Found created collection without parent."));
                return false;
            }

            /* Check if the parent has already been processed. If yes, set the parent of this
             * collection and mark this one as done. Otherwise a second pass will be needed later. */
            if (parentIt->isProcessed()) {
                qCDebugNC(EWSRES_LOG) << QStringLiteral("Processing");
                it->collection.setParentCollection(parentIt->collection);
                it->flags |= FolderDescr::Processed;
            } else {
                qCDebugNC(EWSRES_LOG) << QStringLiteral("Parent not yet processed - delaying");
                reparentPassNeeded = true;
            }
        } else {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Collection is not remotely changed");
            /* This is either a deleted folder or a parent to an added collection. No processing
             * needed for either of those. */
            it->flags |= FolderDescr::Processed;
        }

        /* Check if this collection is a top-level collection. */
        if (!mFolderHash.contains(it->parent())) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Collection is top level");
            topLevelList.append(it.key());
        }

        /* Put the collection into the parent map. This will help running the reparent pass. */
        if (!it->parent().isNull()) {
            mParentMap.insert(it->parent(), it.key());
        }
    }

    if (reparentPassNeeded) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Executing reparent pass") << topLevelList;
        Q_FOREACH(const QString &id, topLevelList) {
            reparentRemoteFolder(id);
        }
    }

    /* Build the resulting collection list. */
    for (auto it = mFolderHash.begin(); it != mFolderHash.end(); ++it) {
        if (it->isRemoved()) {
            q->mDeletedFolders.append(it->collection);
        } else if (it->isProcessed()) {
            q->mChangedFolders.append(it->collection);
        } else {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Found unprocessed collection %1").arg(it.key());
            return false;
        }
    }

    return true;
}

void EwsFetchFoldersIncrJobPrivate::reparentRemoteFolder(const QString &id)
{
    qCDebugNC(EWSRES_LOG) << QStringLiteral("Reparenting") << id;
    QStringList children = mParentMap.values(id);
    FolderDescr &fd = mFolderHash[id];
    Q_FOREACH(const QString &childId, children) {
        FolderDescr &childFd = mFolderHash[childId];
        if (!childFd.isProcessed() && childFd.isModified() &&
            childFd.parent() != childFd.collection.parentCollection().remoteId()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found moved collection");
            /* Found unprocessed collection move. */
            moveCollection(childFd);
        }

        childFd.collection.setParentCollection(fd.collection);
        reparentRemoteFolder(childId);
    }
    fd.flags |= FolderDescr::Processed;
}

void EwsFetchFoldersIncrJobPrivate::moveCollection(const FolderDescr &fd)
{
    qCDebugNC(EWSRES_LOG) << QStringLiteral("Moving collection") << fd.collection.remoteId() <<
                    QStringLiteral("from") << fd.collection.parentCollection().remoteId() <<
                    QStringLiteral("to") << fd.parent();
    CollectionMoveJob *job = new CollectionMoveJob(fd.collection, mFolderHash[fd.parent()].collection);
    connect(job, &CollectionMoveJob::result, this, &EwsFetchFoldersIncrJobPrivate::localFolderMoveDone);
    mPendingMoveJobs++;
    job->start();
}

void EwsFetchFoldersIncrJobPrivate::localFolderMoveDone(KJob *job)
{
    Q_Q(EwsFetchFoldersIncrJob);

    if (job->error()) {
        q->setErrorMsg(QStringLiteral("Failed to move collection."));
        q->emitResult();
        return;
    }

    if (--mPendingMoveJobs == 0) {
        q->emitResult();
    }
}

void EwsFetchFoldersIncrJobPrivate::updateFolderCollection(Collection &collection, const EwsFolder &folder)
{
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
    collection.setRemoteRevision(id.changeKey());
}


EwsFetchFoldersIncrJob::EwsFetchFoldersIncrJob(EwsClient &client, const QString &syncState,
                                       const Akonadi::Collection &rootCollection, QObject *parent)
    : EwsJob(parent), mSyncState(syncState),
      d_ptr(new EwsFetchFoldersIncrJobPrivate(this, client, rootCollection))
{
    qRegisterMetaType<EwsId::List>();
}

EwsFetchFoldersIncrJob::~EwsFetchFoldersIncrJob()
{
}

void EwsFetchFoldersIncrJob::start()
{
    Q_D(const EwsFetchFoldersIncrJob);

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
            &EwsFetchFoldersIncrJobPrivate::remoteFolderIncrFetchDone);
    // Don't add this as a subjob as the error is handled in its own way rather than throwing an
    // error code to the parent.

    syncFoldersReq->start();
}

QDebug operator<<(QDebug debug, const FolderDescr &fd)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("FolderDescr(");

    d << fd.collection;
    d << fd.flags;

    d << ')';
    return debug;
}
