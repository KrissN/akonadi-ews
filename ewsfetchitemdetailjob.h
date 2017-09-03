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

#ifndef EWSFETCHITEMDETAILJOB_H
#define EWSFETCHITEMDETAILJOB_H

#include <QPointer>

#include <KCoreAddons/KCompositeJob>

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

#include "ewsclient.h"
#include "ewsgetitemrequest.h"
#include "ewsid.h"
#include "ewsitem.h"
#include "ewstypes.h"

class EwsFetchItemDetailJob : public KCompositeJob
{
    Q_OBJECT
public:
    EwsFetchItemDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection);
    virtual ~EwsFetchItemDetailJob();

    void setItemLists(Akonadi::Item::List changedItems, Akonadi::Item::List *deletedItems);

    Akonadi::Item::List changedItems() const
    {
        return mChangedItems;
    }

    virtual void start() Q_DECL_OVERRIDE;
protected:
    virtual void processItems(const QList<EwsGetItemRequest::Response> &responses) = 0;

    QPointer<EwsGetItemRequest> mRequest;
    Akonadi::Item::List mChangedItems;
    Akonadi::Item::List *mDeletedItems;
    EwsClient &mClient;
    const Akonadi::Collection mCollection;
private Q_SLOTS:
    void itemDetailFetched(KJob *job);
private:
};

#endif
