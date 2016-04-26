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

#include "ewssubscribedfoldersjob.h"

#include "ewsclient.h"
#include "ewsgetfolderrequest.h"
#include "settings.h"
#include "ewsclient_debug.h"

EwsSubscribedFoldersJob::EwsSubscribedFoldersJob(EwsClient &client, QObject *parent)
    : EwsJob(parent), mClient(client)
{

}

EwsSubscribedFoldersJob::~EwsSubscribedFoldersJob()
{
}

void EwsSubscribedFoldersJob::start()
{
    EwsId::List ids;

    // Before subscribing make sure the subscription list doesn't contain invalid folders.
    // Do this also for the default list in order to transform the distinguished IDs into real ones.
    if (Settings::serverSubscriptionList() == QStringList() << "default") {
        ids = defaultSubscriptionFolders();
    } else {
        Q_FOREACH(const QString &id, Settings::serverSubscriptionList()) {
            ids << EwsId(id);
        }
    }

    EwsGetFolderRequest *req = new EwsGetFolderRequest(mClient, this);
    req->setFolderShape(EwsFolderShape(EwsShapeIdOnly));
    req->setFolderIds(ids);
    req->setProperty("ids", QVariant::fromValue<EwsId::List>(ids));
    connect(req, &EwsRequest::result, this, &EwsSubscribedFoldersJob::verifySubFoldersRequestFinished);
    req->start();
}

void EwsSubscribedFoldersJob::verifySubFoldersRequestFinished(KJob *job)
{
    if (!job->error()) {
        EwsGetFolderRequest *req = qobject_cast<EwsGetFolderRequest*>(job);
        Q_ASSERT(req);

        mFolders.clear();
        EwsId::List sourceIds = req->property("ids").value<EwsId::List>();
        QStringList idList;

        Q_ASSERT(req->responses().size() == sourceIds.size());

        auto it = sourceIds.cbegin();

        Q_FOREACH(const EwsGetFolderRequest::Response &resp, req->responses()) {
            if (resp.isSuccess()) {
                // Take just the id without the change key as the actual folder version is irrelevant
                // here
                QString id = resp.folder()[EwsFolderFieldFolderId].value<EwsId>().id();
                mFolders << EwsId(id);
                idList << id;
            } else {
                qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid folder %1 - skipping").arg(it->id());
            }
            it++;
        }

        // Once verified write the final list back to the configuration.
        Settings::setServerSubscriptionList(idList);
    } else {
        setErrorMsg(job->errorString());
    }
    emitResult();
}

const EwsId::List &EwsSubscribedFoldersJob::defaultSubscriptionFolders()
{
    static const EwsId::List list = {EwsId(EwsDIdInbox), EwsId(EwsDIdCalendar), EwsId(EwsDIdTasks),
        EwsId(EwsDIdContacts)};

    return list;
}

