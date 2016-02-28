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

#include "ewsmodifymailjob.h"

#include <Akonadi/KMime/MessageFlags>

#include "ewsupdateitemrequest.h"

#include "ewsclient_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidFlagStatus(0x1090, EwsPropTypeInteger);

EwsModifyMailJob::EwsModifyMailJob(EwsClient& client, const Akonadi::Item &item,
                                   const QSet<QByteArray> &parts, QObject *parent)
    : EwsModifyItemJob(client, item, parts, parent)
{
}
EwsModifyMailJob::~EwsModifyMailJob()
{
}

void EwsModifyMailJob::start()
{
    bool doSubmit = false;
    EwsUpdateItemRequest *req = new EwsUpdateItemRequest(mClient, this);
    EwsId itemId(mItem.remoteId(), mItem.remoteRevision());

    if (mParts.contains("FLAGS")) {
        EwsUpdateItemRequest::ItemChange ic(itemId, EwsItemTypeMessage);
        qDebug() << "Item flags" << mItem.flags();
        bool isRead = mItem.flags().contains(MessageFlags::Seen);
        EwsUpdateItemRequest::Update *upd =
                        new EwsUpdateItemRequest::SetUpdate(EwsPropertyField(QStringLiteral("message:IsRead")),
                                                            isRead ? QStringLiteral("true") : QStringLiteral("false"));
        ic.addUpdate(upd);
        bool isFlagged = mItem.flags().contains(MessageFlags::Flagged);
        if (isFlagged) {
            upd = new EwsUpdateItemRequest::SetUpdate(propPidFlagStatus, QStringLiteral("2"));
        }
        else {
            upd = new EwsUpdateItemRequest::DeleteUpdate(propPidFlagStatus);
        }
        ic.addUpdate(upd);
        req->addItemChange(ic);
        doSubmit = true;
    }

    if (doSubmit) {
        connect(req, SIGNAL(result(KJob*)), SLOT(updateItemFinished(KJob*)));
        req->start();
    }
    else {
        delete req;
        qDebug() << "Nothing to do for parts" << mParts;
        emitResult();
    }
}

void EwsModifyMailJob::updateItemFinished(KJob *job)
{
    if (job->error()) {
        setErrorText(job->errorString());
        emitResult();
        return;
    }

    EwsUpdateItemRequest *req = qobject_cast<EwsUpdateItemRequest*>(job);
    if (!req) {
        setErrorText(QStringLiteral("Invalid EwsUpdateItemRequest job object"));
        emitResult();
        return;
    }

    EwsUpdateItemRequest::Response resp = req->responses().first();
    if (!resp.isSuccess()) {
        setErrorText(QStringLiteral("Item update failed: ") + resp.responseMessage());
        emitResult();
        return;
    }

    Item item = job->property("item").value<Item>();
    if (item.isValid()) {
        item.setRemoteRevision(resp.itemId().changeKey());
    }

    emitResult();
}
