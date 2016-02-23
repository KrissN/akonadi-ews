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

#include "ewscreatemailjob.h"

#include <KMime/Message>
#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KI18n/KLocalizedString>

#include "ewscreateitemrequest.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidMessageFlags(0x0e07, EwsPropTypeInteger);

EwsCreateMailJob::EwsCreateMailJob(EwsClient& client, const Akonadi::Item &item,
                                   const Akonadi::Collection &collection, QObject *parent)
    : EwsCreateItemJob(client, item, collection, parent)
{
}
EwsCreateMailJob::~EwsCreateMailJob()
{
}

void EwsCreateMailJob::start()
{
    if (!mItem.hasPayload<KMime::Message::Ptr>()) {
        setErrorMsg(QStringLiteral("Expected MIME message payload"));
        emitResult();
    }

    EwsCreateItemRequest *req = new EwsCreateItemRequest(mClient, this);

    KMime::Message::Ptr msg = mItem.payload<KMime::Message::Ptr>();
    QByteArray mimeContent = msg->encodedContent(true);
    EwsItem item;
    item.setType(EwsItemTypeMessage);
    item.setField(EwsItemFieldMimeContent, mimeContent);
    /* When creating items using the CreateItem request Exchange will by default mark the  message
     * as draft. Setting the extended property below causes the message to appear normally. */
    item.setProperty(propPidMessageFlags, QStringLiteral("1"));
    req->setItems(EwsItem::List() << item);
    req->setMessageDisposition(EwsDispSaveOnly);
    req->setSavedFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
    connect(req, &EwsCreateItemRequest::finished, this, &EwsCreateMailJob::mailCreateFinished);
    addSubjob(req);
    req->start();
}

void EwsCreateMailJob::mailCreateFinished(KJob *job)
{
    qDebug() << "mailCreateFinished";
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    EwsCreateItemRequest *req = qobject_cast<EwsCreateItemRequest*>(job);
    if (!req) {
        setErrorMsg(QStringLiteral("Invalid job object"));
        emitResult();
        return;
    }

    if (req->responses().count() != 1) {
        setErrorMsg(QStringLiteral("Invalid number of responses received from server."));
        emitResult();
        return;
    }

    EwsCreateItemRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        EwsId id = resp.itemId();
        mItem.setRemoteId(id.id());
        mItem.setRemoteRevision(id.changeKey());
        mItem.setParentCollection(mCollection);
    }
    else {
        setErrorMsg(i18n("Failed to create mail item"));
    }

    emitResult();
}
