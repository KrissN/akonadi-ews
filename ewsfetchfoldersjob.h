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
                       const Akonadi::Collection &rootCollection, QObject *parent);
    virtual ~EwsFetchFoldersJob();

    Akonadi::Collection::List changedFolders() const { return mChangedFolders; };
    Akonadi::Collection::List deletedFolders() const { return mDeletedFolders; };
    Akonadi::Collection::List folders() const { return mFolders; };
    const QString &syncState() const { return mSyncState; };
    bool fullSync() const { return mFullSync; };

    virtual void start() Q_DECL_OVERRIDE;
private Q_SLOTS:
    void remoteFolderFullFetchDone(KJob *job);
    void remoteFolderIncrFetchDone(KJob *job);
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    Akonadi::Collection createFolderCollection(const EwsFolder &folder);

    EwsClient& mClient;
    const Akonadi::Collection &mRootCollection;
    EwsFolder::List mRemoteAddedFolders;
    EwsFolder::List mRemoteChangedFolders;
    EwsId::List mRemoteDeletedIds;
    QString mSyncState;
    bool mFullSync;

    Akonadi::Collection::List mChangedFolders;
    Akonadi::Collection::List mDeletedFolders;
    Akonadi::Collection::List mFolders;
};

#endif
