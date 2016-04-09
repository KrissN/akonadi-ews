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

#ifndef EWSUPDATEITEMSTAGSJOB_H
#define EWSUPDATEITEMSTAGSJOB_H

#include "ewsjob.h"
#include <AkonadiCore/Item>

class EwsTagStore;
class EwsClient;
class EwsResource;

/**
 *  @brief  Job used to update Exchange items wit tag information from Akonadi
 *
 *  This job cycles through all items and updates the Exchange database with tag information from
 *  the items.
 *
 *  The job relies on the tag store to retrieve tag identifiers and names that can be stored in
 *  Exchange. Due to buggy tag implementation in Akonad it can happen that items contain tags not
 *  yet known to the EWS resource. In such case an additional tag fetch job is issues to fetch
 *  information about those tags so that they can be added to the tag store.
 */
class EwsUpdateItemsTagsJob : public EwsJob
{
    Q_OBJECT
public:
    EwsUpdateItemsTagsJob(const Akonadi::Item::List &items, EwsTagStore *tagStore, EwsClient &client,
                          EwsResource *parent);
    ~EwsUpdateItemsTagsJob();

    void start() Q_DECL_OVERRIDE;

    Akonadi::Item::List items() { return mItems; };
private Q_SLOTS:
    void itemsTagsChangedTagsFetched(KJob *job);
    void updateItemsTagsRequestFinished(KJob *job);
    void globalTagsWriteFinished(KJob *job);
private:
    void doUpdateItemsTags();

    Akonadi::Item::List mItems;
    EwsTagStore *mTagStore;
    EwsClient &mClient;
};

#endif
