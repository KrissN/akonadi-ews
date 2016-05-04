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

#include "ewssyncfolderhierarchyrequest.h"
#include "ewseffectiverights.h"
#include "ewsclient.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidTagContainerClass(0x3613, EwsPropTypeString);

EwsFetchFoldersJob::EwsFetchFoldersJob(EwsClient &client, const QString &syncState,
                                       const Akonadi::Collection &rootCollection, QObject *parent)
    : EwsJob(parent), mClient(client), mRootCollection(rootCollection), mSyncState(syncState),
      mFullSync(syncState.isNull())
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
    addSubjob(syncFoldersReq);

    syncFoldersReq->start();
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
        return;
    }

    QHash<QString, Collection> map;

    mFolders.append(mRootCollection);
    map.insert(mRootCollection.remoteId(), mRootCollection);

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        if (ch.type() == EwsSyncFolderHierarchyRequest::Create) {
            Collection collection = createFolderCollection(ch.folder());

            EwsId parentId = ch.folder()[EwsFolderFieldParentFolderId].value<EwsId>();
            QHash<QString, Collection>::iterator it = map.find(parentId.id());
            if (it == map.end()) {
                qCWarningNC(EWSRES_LOG) << QStringLiteral("No collection found for ") << parentId;
                setErrorMsg(QStringLiteral("No collection found for %1").arg(parentId.id()));
                emitResult();
                return;
            }
            collection.setParentCollection(*it);

            mFolders.append(collection);
            map.insert(collection.remoteId(), collection);
        }
        else {
            setErrorMsg(QStringLiteral("Got non-create change for full sync."));
            emitResult();
            return;
        }
    }
    mSyncState = req->syncState();

    emitResult();
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

    QHash<QString, Collection> map;
    QHash<QString, Collection> dummyMap;
    QMultiHash<QString, Collection> reparentMap;

    Q_FOREACH(const EwsSyncFolderHierarchyRequest::Change &ch, req->changes()) {
        switch (ch.type()) {
        case EwsSyncFolderHierarchyRequest::Create:
        case EwsSyncFolderHierarchyRequest::Update:
        {
            Collection c = createFolderCollection(ch.folder());

            /* Some previously processed collection might have used this collection as a parent. At
             * that time full details about this collections were not known and a dummy item was used
             * as the parent. Now check if this item has appeared in full detail and if yes, replace
             * the dummy parent with the full one. */
            QMultiHash<QString, Collection>::iterator reparentIt = reparentMap.find(c.remoteId());
            for (; reparentIt != reparentMap.end(); reparentIt++) {
                Collection rc = *reparentIt;
                mChangedFolders.removeOne(rc);
                dummyMap.remove(c.remoteId());
                rc.setParentCollection(c);
                mChangedFolders.append(c);
            }

            /* Set the parent collection. If the parent collection was already processed then use
             * it. Otherwise create a dummy collection. */
            EwsId parentId = ch.folder()[EwsFolderFieldParentFolderId].value<EwsId>();
            QHash<QString, Collection>::iterator it = map.find(parentId.id());
            if (it != map.end()) {
                c.setParentCollection(*it);
            }
            else {
                QHash<QString, Collection>::iterator it = dummyMap.find(parentId.id());
                Collection parent;
                if (it != dummyMap.end()) {
                    parent = *it;
                }
                else {
                    parent.setRemoteId(parentId.id());
                    dummyMap.insert(parentId.id(), parent);
                }

                c.setParentCollection(parent);
                reparentMap.insert(c.remoteId(), c);
            }
            EwsId id = ch.folder()[EwsFolderFieldFolderId].value<EwsId>();
            map.insert(id.id(), c);
            mChangedFolders.append(c);
            break;
        }
        case EwsSyncFolderHierarchyRequest::Delete:
        {
            Collection c;
            c.setRemoteId(ch.folderId().id());
            mDeletedFolders.append(c);
            break;
        }
        default:
            break;
        }
    }
    mSyncState = req->syncState();

    emitResult();
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
