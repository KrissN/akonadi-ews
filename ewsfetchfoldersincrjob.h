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

#ifndef EWSFETCHFOLDERSINCRJOB_H
#define EWSFETCHFOLDERSINCRJOB_H

#include <QScopedPointer>

#include <AkonadiCore/Collection>

#include "ewsjob.h"
#include "ewsfolder.h"

class EwsClient;
class EwsFetchFoldersIncrJobPrivate;

class EwsFetchFoldersIncrJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchFoldersIncrJob(EwsClient &client, const QString &syncState,
                           const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsFetchFoldersIncrJob() override;

    Akonadi::Collection::List changedFolders() const
    {
        return mChangedFolders;
    }
    Akonadi::Collection::List deletedFolders() const
    {
        return mDeletedFolders;
    }
    const QString &syncState() const
    {
        return mSyncState;
    }

    void start() override;
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    Akonadi::Collection::List mChangedFolders;
    Akonadi::Collection::List mDeletedFolders;

    QString mSyncState;

    QScopedPointer<EwsFetchFoldersIncrJobPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsFetchFoldersIncrJob)
};

#endif
