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

#include "ewsakonaditagssyncjob.h"

#include <AkonadiCore/TagFetchJob>
#include "ewstagstore.h"
#include "ewsglobaltagswritejob.h"

using namespace Akonadi;

EwsAkonadiTagsSyncJob::EwsAkonadiTagsSyncJob(EwsTagStore *tagStore, EwsClient &client,
                                             const Collection &rootCollection, QObject *parent)
    : EwsJob(parent), mTagStore(tagStore), mClient(client), mRootCollection(rootCollection)
{
}

EwsAkonadiTagsSyncJob::~EwsAkonadiTagsSyncJob()
{
}

void EwsAkonadiTagsSyncJob::start()
{
    TagFetchJob *job = new TagFetchJob(this);
    connect(job, &TagFetchJob::result, this, &EwsAkonadiTagsSyncJob::tagFetchFinished);
}

void EwsAkonadiTagsSyncJob::tagFetchFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    TagFetchJob *tagJob = qobject_cast<TagFetchJob*>(job);
    Q_ASSERT(tagJob);

    if (mTagStore->syncTags(tagJob->tags())) {
        EwsGlobalTagsWriteJob *tagJob = new EwsGlobalTagsWriteJob(mTagStore, mClient, mRootCollection, this);
        connect(tagJob, &EwsGlobalTagsWriteJob::result, this, &EwsAkonadiTagsSyncJob::tagWriteFinished);
        tagJob->start();
    } else {
        emitResult();
    }
}
void EwsAkonadiTagsSyncJob::tagWriteFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
    }

    emitResult();
}
