/*
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
#include "ewsmailhandler.h"

#include "ewsresource_debug.h"

using namespace Akonadi;

EwsModifyMailJob::EwsModifyMailJob(EwsClient &client, const Akonadi::Item::List &items,
                                   const QSet<QByteArray> &parts, QObject *parent)
    : EwsModifyItemJob(client, items, parts, parent)
{
}
EwsModifyMailJob::~EwsModifyMailJob()
{
}

void EwsModifyMailJob::start()
{
    bool doSubmit = false;
    EwsUpdateItemRequest *req = new EwsUpdateItemRequest(mClient, this);
    EwsId itemId;

    Q_FOREACH (const Item &item, mItems) {
        itemId = EwsId(item.remoteId(), item.remoteRevision());

        if (mParts.contains("FLAGS")) {
            EwsUpdateItemRequest::ItemChange ic(itemId, EwsItemTypeMessage);
            QHash<EwsPropertyField, QVariant> propertyHash = EwsMailHandler::writeFlags(item.flags());

            for (auto it = propertyHash.cbegin(); it != propertyHash.cend(); ++it) {
                EwsUpdateItemRequest::Update *upd;
                if (it.value().isNull()) {
                    upd = new EwsUpdateItemRequest::DeleteUpdate(it.key());
                } else {
                    upd = new EwsUpdateItemRequest::SetUpdate(it.key(), it.value());
                }
                ic.addUpdate(upd);
            }

            req->addItemChange(ic);
            doSubmit = true;
        }
    }

    if (doSubmit) {
        connect(req, SIGNAL(result(KJob*)), SLOT(updateItemFinished(KJob*)));
        req->start();
    } else {
        delete req;
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

    Q_ASSERT(req->responses().size() == mItems.size());

    Item::List::iterator it = mItems.begin();
    Q_FOREACH (const EwsUpdateItemRequest::Response &resp, req->responses()) {
        if (!resp.isSuccess()) {
            setErrorText(QStringLiteral("Item update failed: ") + resp.responseMessage());
            emitResult();
            return;
        }

        it->setRemoteRevision(resp.itemId().changeKey());
    }

    emitResult();
}
