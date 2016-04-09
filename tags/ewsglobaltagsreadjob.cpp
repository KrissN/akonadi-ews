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

#include "ewsglobaltagsreadjob.h"

#include "ewsclient.h"
#include "ewsgetfolderrequest.h"
#include "ewstagstore.h"
#include "ewsresource.h"

#include "ewsclient_debug.h"

using namespace Akonadi;

EwsGlobalTagsReadJob::EwsGlobalTagsReadJob(EwsTagStore *tagStore, EwsClient &client,
                                           const Collection &rootCollection, QObject *parent)
    : EwsJob(parent), mTagStore(tagStore), mClient(client), mRootCollection(rootCollection)
{
}

EwsGlobalTagsReadJob::~EwsGlobalTagsReadJob()
{
}

void EwsGlobalTagsReadJob::start()
{
    EwsGetFolderRequest *req = new EwsGetFolderRequest(mClient, this);
    req->setFolderIds(EwsId::List() << EwsId(EwsDIdMsgFolderRoot));
    EwsFolderShape shape(EwsShapeIdOnly);
    shape << EwsResource::globalTagsProperty << EwsResource::globalTagsVersionProperty;
    req->setFolderShape(shape);
    connect(req, &EwsGetFolderRequest::result, this, &EwsGlobalTagsReadJob::getFolderRequestFinished);
    req->start();
}

void EwsGlobalTagsReadJob::getFolderRequestFinished(KJob *job)
{
    EwsGetFolderRequest *req = qobject_cast<EwsGetFolderRequest*>(job);

    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsGetFolderRequest job object");
        setErrorMsg(QStringLiteral("Invalid EwsGetFolderRequest job object"));
        emitResult();
        return;
    }

    if (req->error()) {
        setErrorMsg(req->errorString());
        emitResult();
        return;
    }

    if (req->responses().size() != 1) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid number of responses received");
        setErrorMsg(QStringLiteral("Invalid number of responses received"));
        emitResult();
        return;
    }

    EwsFolder folder = req->responses().first().folder();
    bool status = mTagStore->readTags(folder[EwsResource::globalTagsProperty].toStringList(),
        folder[EwsResource::globalTagsVersionProperty].toInt());
    if (!status) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Incorrect server tag data");
        setErrorMsg(QStringLiteral("Incorrect server tag data"));
        emitResult();
    }

    mTags = mTagStore->tags();

    emitResult();
}
