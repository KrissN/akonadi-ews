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

#include "ewsfetchitemdetailjob.h"

#include "ewsgetitemrequest.h"

QList<EwsFetchItemDetailJob::JobFactory> EwsFetchItemDetailJob::mJobFactories;

EwsFetchItemDetailJob::EwsFetchItemDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection)
    : KCompositeJob(parent), mDeletedItems(Q_NULLPTR), mClient(client), mCollection(collection)
{
    mRequest = new EwsGetItemRequest(client, this);
    connect(mRequest, SIGNAL(result(KJob*)), SLOT(itemDetailFetched(KJob*)));
    addSubjob(mRequest);
}

EwsFetchItemDetailJob::~EwsFetchItemDetailJob()
{
}

void EwsFetchItemDetailJob::setItemLists(Akonadi::Item::List changedItems,
                                         Akonadi::Item::List *deletedItems)
{
    mChangedItems = changedItems;
    mDeletedItems = deletedItems;

    EwsId::List ids;

    Q_FOREACH(const Akonadi::Item &item, changedItems)
    {
        EwsId id(item.remoteId(), item.remoteRevision());
        ids.append(id);
    }

    mRequest->setItemIds(ids);
}

void EwsFetchItemDetailJob::itemDetailFetched(KJob *job)
{
    if (!job->error() && job == mRequest) {
        Q_ASSERT(mChangedItems.size() == mRequest->responses().size());

        processItems(mRequest->responses());
    }
}

void EwsFetchItemDetailJob::start()
{
    qDebug() << "started";
    mRequest->start();
}

void EwsFetchItemDetailJob::registerItemDetailFetchJob(EwsItemType type,
                                                       FetchItemDetailJobFactory factory)
{
    mJobFactories.append({type, factory});
}
EwsFetchItemDetailJob *EwsFetchItemDetailJob::createFetchItemDetailJob(EwsItemType type,
                                                                       EwsClient &client,
                                                                       QObject *parent,
                                                                       const Akonadi::Collection &collection)
{
    Q_FOREACH (const JobFactory& factory, mJobFactories) {
        if (factory.type == type) {
            return factory.factory(client, parent, collection);
        }
    }

    return Q_NULLPTR;
}
