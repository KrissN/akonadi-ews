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

#include "ewscreateitemjob.h"

#include "ewsresource.h"
#include "tags/ewstagstore.h"
#include "tags/ewsakonaditagssyncjob.h"

EwsCreateItemJob::EwsCreateItemJob(EwsClient& client, const Akonadi::Item &item,
                                   const Akonadi::Collection &collection, EwsTagStore *tagStore,
                                   EwsResource *parent)
    : EwsJob(parent), mItem(item), mCollection(collection), mClient(client), mTagStore(tagStore)
{
}

EwsCreateItemJob::~EwsCreateItemJob()
{
}

const Akonadi::Item &EwsCreateItemJob::item() const
{
    return mItem;
}

void EwsCreateItemJob::start()
{
    /* Before starting check if all Akonadi tags are known to the tag store */
    bool syncNeeded = false;
    Q_FOREACH(const Akonadi::Tag &tag, mItem.tags()) {
        if (!mTagStore->containsId(tag.id())) {
            syncNeeded = true;
            break;
        }
    }

    if (syncNeeded) {
        qDebug() << "EwsCreateItemJob: sync needed";
        EwsAkonadiTagsSyncJob *job = new EwsAkonadiTagsSyncJob(mTagStore,
            mClient, qobject_cast<EwsResource*>(parent())->rootCollection(), this);
        connect(job, &EwsAkonadiTagsSyncJob::result, this, &EwsCreateItemJob::tagSyncFinished);
        job->start();
    } else {
        doStart();
    }
}

void EwsCreateItemJob::populateCommonProperties(EwsItem &item)
{
    if (!mTagStore->writeEwsProperties(mItem, item)) {
        setErrorMsg(QStringLiteral("Failed to write tags despite an earlier sync"));
    }
}

void EwsCreateItemJob::tagSyncFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorText());
        emitResult();
    } else {
        doStart();
    }
}
