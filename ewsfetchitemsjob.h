/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

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

class EwsFetchItemsJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchItemsJob(const Akonadi::Collection &collection, EwsClient& client, QObject *parent);
    virtual ~EwsFetchItemsJob();

    Akonadi::Item::List changedItems() const { return mChangedItems; };
    Akonadi::Item::List deletedItems() const { return mDeletedItems; };

    virtual void start() Q_DECL_OVERRIDE;
private Q_SLOTS:
    void localItemFetchDone(KJob *job);
    void remoteItemFetchDone(KJob *job);
private:
    void compareItemLists();

    const Akonadi::Collection mCollection;
    EwsClient& mClient;
    Akonadi::Item::List mLocalItems;
    QList<EwsItem> mRemoteItems;
    int mJobsFinished;

    Akonadi::Item::List mChangedItems;
    Akonadi::Item::List mDeletedItems;
};

#endif
