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

#include "ewsupdateitemstagsjob.h"

#include <AkonadiCore/Tag>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagAttribute>
#include <AkonadiCore/AttributeFactory>

#include "ewsid.h"
#include "ewsupdateitemrequest.h"
#include "ewsitemhandler.h"
#include "ewsclient.h"
#include "ewstagstore.h"
#include "ewsresource.h"
#include "ewsglobaltagswritejob.h"

using namespace Akonadi;

EwsUpdateItemsTagsJob::EwsUpdateItemsTagsJob(const Akonadi::Item::List &items, EwsTagStore *tagStore,
                                             EwsClient &client, EwsResource *parent)
    : EwsJob(parent), mItems(items), mTagStore(tagStore), mClient(client)
{
}

EwsUpdateItemsTagsJob::~EwsUpdateItemsTagsJob()
{
}

void EwsUpdateItemsTagsJob::start()
{
    Tag::List unknownTags;

    /* In the Exchange world it is not possible to add or remove individual tags from an item
     * - it is necessary to write the full list of tags at once. Unfortunately the tags objects
     * attached to an item only contain the id. They're missing any persistent identification such
     * as the uid. If the EWS resource hasn't seen these tags yet it is necessary to fetch them
     * first before any further processing.
     */
    Q_FOREACH(const Item &item, mItems) {
        qDebug() << item.tags().size();
        Q_FOREACH(const Tag &tag, item.tags()) {
            qDebug() << tag.name() << tag.url() << tag.gid() << tag.id();
            if (!mTagStore->containsId(tag.id())) {
                unknownTags.append(tag);
            }
        }
    }

    if (!unknownTags.empty()) {
        TagFetchJob *job = new TagFetchJob(unknownTags, this);
        connect(job, &TagFetchJob::result, this, &EwsUpdateItemsTagsJob::itemsTagsChangedTagsFetched);
    } else {
        doUpdateItemsTags();
    }
}

void EwsUpdateItemsTagsJob::itemsTagsChangedTagsFetched(KJob *job)
{
    Item::List items = job->property("items").value<Item::List>();

    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    TagFetchJob *tagJob = qobject_cast<TagFetchJob*>(job);
    if (!tagJob) {
        setErrorMsg(QStringLiteral("Invalid TagFetchJob job object"));
        emitResult();
        return;
    }

    /* All unknown tags have been fetched and can be written to Exchange. */
    mTagStore->addTags(tagJob->tags());

    EwsResource *res = qobject_cast<EwsResource*>(parent());
    Q_ASSERT(res);
    EwsGlobalTagsWriteJob *writeJob = new EwsGlobalTagsWriteJob(mTagStore, mClient,
        res->rootCollection(), this);
    connect(writeJob, &EwsGlobalTagsWriteJob::result, this,
            &EwsUpdateItemsTagsJob::globalTagsWriteFinished);
    writeJob->start();
}

void EwsUpdateItemsTagsJob::globalTagsWriteFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    doUpdateItemsTags();
}

void EwsUpdateItemsTagsJob::doUpdateItemsTags()
{
    EwsUpdateItemRequest *req = new EwsUpdateItemRequest(mClient, this);
    Q_FOREACH(const Item &item, mItems) {
        EwsUpdateItemRequest::ItemChange ic(EwsId(item.remoteId(), item.remoteRevision()),
                                            EwsItemHandler::mimeToItemType(item.mimeType()));
        if (!item.tags().isEmpty()) {
            QStringList tagList;
            QStringList categoryList;
            Q_FOREACH(const Tag &tag, item.tags()) {
                Q_ASSERT(mTagStore->containsId(tag.id()));
                tagList.append(mTagStore->tagRemoteId(tag.id()));
                QString name = mTagStore->tagName(tag.id());
                if (!name.isEmpty()) {
                    categoryList.append(name);
                }
            }
            EwsUpdateItemRequest::Update *upd
                = new EwsUpdateItemRequest::SetUpdate(EwsResource::tagsProperty, tagList);
            ic.addUpdate(upd);
            upd = new EwsUpdateItemRequest::SetUpdate(EwsPropertyField("item:Categories"), categoryList);
            ic.addUpdate(upd);
        } else {
            EwsUpdateItemRequest::Update *upd
                            = new EwsUpdateItemRequest::DeleteUpdate(EwsResource::tagsProperty);
            ic.addUpdate(upd);
            upd = new EwsUpdateItemRequest::DeleteUpdate(EwsPropertyField("item:Categories"));
            ic.addUpdate(upd);
        }
        req->addItemChange(ic);
    }

    connect(req, &EwsUpdateItemRequest::result, this,
            &EwsUpdateItemsTagsJob::updateItemsTagsRequestFinished);
    req->start();
}

void EwsUpdateItemsTagsJob::updateItemsTagsRequestFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    EwsUpdateItemRequest *req = qobject_cast<EwsUpdateItemRequest*>(job);
    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsUpdateItemRequest job object"));
        return;
    }

    Q_ASSERT(mItems.count() == req->responses().count());

    auto itemIt = mItems.begin();
    Q_FOREACH(const EwsUpdateItemRequest::Response &resp, req->responses()) {
        if (resp.isSuccess()) {
            itemIt->setRemoteRevision(resp.itemId().changeKey());
        }
    }

    emitResult();
}
