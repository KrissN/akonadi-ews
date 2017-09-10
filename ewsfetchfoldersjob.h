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

#ifndef EWSFETCHFOLDERSJOB_H
#define EWSFETCHFOLDERSJOB_H

#include <QScopedPointer>

#include <AkonadiCore/Collection>

#include "ewsjob.h"
#include "ewsfolder.h"

class EwsClient;
class EwsFetchFoldersJobPrivate;

class EwsFetchFoldersJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchFoldersJob(EwsClient& client, const Akonadi::Collection &rootCollection, QObject *parent);
    virtual ~EwsFetchFoldersJob();

    Akonadi::Collection::List folders() const { return mFolders; };
    const QString &syncState() const { return mSyncState; };

    virtual void start() Q_DECL_OVERRIDE;
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    Akonadi::Collection::List mFolders;

    QString mSyncState;

    QScopedPointer<EwsFetchFoldersJobPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsFetchFoldersJob)
};

#endif
