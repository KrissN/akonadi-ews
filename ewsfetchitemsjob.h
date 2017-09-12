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

#ifndef EWSFETCHITEMSJOB_H
#define EWSFETCHITEMSJOB_H

#include <AkonadiCore/ItemFetchJob>

#include "ewsjob.h"
#include "ewsfinditemrequest.h"

namespace Akonadi {
class Collection;
}
class EwsClient;
class EwsTagStore;
class EwsResource;

class EwsFetchItemsJob : public EwsJob
{
    Q_OBJECT
public:
    struct QueuedUpdate {
        QString id;
        QString changeKey;
        EwsEventType type;
    };

    typedef QList<QueuedUpdate> QueuedUpdateList;

    EwsFetchItemsJob(const Akonadi::Collection &collection, EwsClient& client,
                     const QString &syncState, const EwsId::List &itemsToCheck,
                     EwsTagStore *tagStore, EwsResource *parent);
    virtual ~EwsFetchItemsJob();

    Akonadi::Item::List changedItems() const { return mChangedItems; };
    Akonadi::Item::List deletedItems() const { return mDeletedItems; };
    const QString &syncState() const { return mSyncState; };
    const Akonadi::Collection &collection() const { return mCollection; };

    void setQueuedUpdates(const QueuedUpdateList &updates);

    virtual void start() override;
private Q_SLOTS:
    void localItemFetchDone(KJob *job);
    void remoteItemFetchDone(KJob *job);
    void itemDetailFetchDone(KJob *job);
    void checkedItemsFetchFinished(KJob *job);
    void tagSyncFinished(KJob *job);
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    void compareItemLists();
    void syncTags();

    /*struct QueuedUpdateInt {
        QString changeKey;
        EwsEventType type;
    };*/
    typedef QHash<EwsEventType, QHash<QString, QString> > QueuedUpdateHash;

    const Akonadi::Collection mCollection;
    EwsClient& mClient;
    EwsId::List mItemsToCheck;
    Akonadi::Item::List mLocalItems;
    EwsItem::List mRemoteAddedItems;
    EwsItem::List mRemoteChangedItems;
    EwsId::List mRemoteDeletedIds;
    QHash<EwsId, bool> mRemoteFlagChangedIds;
    int mPendingJobs;
    unsigned mTotalItems;
    QString mSyncState;
    bool mFullSync;
    QueuedUpdateHash mQueuedUpdates;
    EwsTagStore *mTagStore;
    bool mTagsSynced;

    Akonadi::Item::List mChangedItems;
    Akonadi::Item::List mDeletedItems;
};

#endif
