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

#include "ewsglobaltagswritejob.h"

#include <AkonadiCore/Collection>

#include "ewsupdatefolderrequest.h"
#include "ewsid.h"
#include "ewsitemhandler.h"
#include "ewstagstore.h"
#include "ewsresource.h"

using namespace Akonadi;

EwsGlobalTagsWriteJob::EwsGlobalTagsWriteJob(EwsTagStore *tagStore, EwsClient &client,
                                             const Collection &rootCollection, QObject *parent)
    : EwsJob(parent), mTagStore(tagStore), mClient(client), mRootCollection(rootCollection)
{
}

EwsGlobalTagsWriteJob::~EwsGlobalTagsWriteJob()
{
}

void EwsGlobalTagsWriteJob::start()
{
    QStringList tagList = mTagStore->serialize();

    EwsUpdateFolderRequest *req = new EwsUpdateFolderRequest(mClient, this);
    EwsUpdateFolderRequest::FolderChange fc(EwsId(mRootCollection.remoteId(), mRootCollection.remoteRevision()),
                                            EwsFolderTypeMail);
    EwsUpdateFolderRequest::Update *upd
        = new EwsUpdateFolderRequest::SetUpdate(EwsResource::globalTagsProperty, tagList);
    fc.addUpdate(upd);
    upd = new EwsUpdateFolderRequest::SetUpdate(EwsResource::globalTagsVersionProperty,
                                                QString::number(mTagStore->version()));
    fc.addUpdate(upd);
    req->addFolderChange(fc);
    connect(req, &EwsUpdateFolderRequest::result, this, &EwsGlobalTagsWriteJob::updateFolderRequestFinished);
    req->start();
}

void EwsGlobalTagsWriteJob::updateFolderRequestFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorText());
    }

    emitResult();
}
