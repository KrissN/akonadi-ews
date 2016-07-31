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

#ifndef EWSFETCHIFOLDERSJOB_H
#define EWSFETCHIFOLDERSJOB_H

#include <AkonadiCore/Collection>

#include "ewsjob.h"
#include "ewsfolder.h"

class EwsClient;

class EwsFetchFoldersJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchFoldersJob(EwsClient& client, const QString &syncState,
                       const Akonadi::Collection &rootCollection,
                       const QHash<QString, QString> &folderTreeCache, QObject *parent);
    virtual ~EwsFetchFoldersJob();

    Akonadi::Collection::List changedFolders() const { return mChangedFolders; };
    Akonadi::Collection::List deletedFolders() const { return mDeletedFolders; };
    Akonadi::Collection::List folders() const { return mFolders; };
    const QString &syncState() const { return mSyncState; };
    bool fullSync() const { return mFullSync; };

    virtual void start() Q_DECL_OVERRIDE;
private Q_SLOTS:
    void remoteFolderFullFetchDone(KJob *job);
    void remoteFolderIdFullFetchDone(KJob *job);
    void remoteFolderDetailFetchDone(KJob *job);
    void remoteFolderIncrFetchDone(KJob *job);
    void localFolderFetchDone(KJob *job);
    void unknownParentCollectionsFetchDone(KJob *job);
    void localCollectionsRetrieved(const Akonadi::Collection::List &collections);
    void localFolderMoveDone(KJob *job);
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    Akonadi::Collection createFolderCollection(const EwsFolder &folder);
    bool processRemoteFolders();
    void buildCollectionList(const Akonadi::Collection::List &unknownCollections);
    void buildChildCollectionList(QHash<QString, Akonadi::Collection> unknownColHash,
                                  const Akonadi::Collection &col);

    EwsClient& mClient;
    const Akonadi::Collection &mRootCollection;
    EwsFolder::List mRemoteChangedFolders;  // Contains details of folders that need update
                                            // (either created or changed)
    QStringList mRemoteDeletedIds;  // Contains IDs of folders that have been deleted.
    QStringList mRemoteChangedIds;  // Contains IDs of folders that have changed (not created).
    QString mSyncState;
    bool mFullSync;
    int mPendingFetchJobs;
    int mPendingMoveJobs;
    EwsId::List mRemoteFolderIds;

    Akonadi::Collection::List mChangedFolders;
    Akonadi::Collection::List mDeletedFolders;
    Akonadi::Collection::List mFolders;

    //QHash<QString, Akonadi::Collection> mDummyMap;
    //QMultiHash<QString, Akonadi::Collection> mReparentMap;
    const QHash<QString, QString> &mFolderTreeCache;
    QList<QString> mUnknownParentList;

    QHash<QString, Akonadi::Collection> mCollectionMap;
    QMultiHash<QString, QString> mParentMap;

};

#endif
