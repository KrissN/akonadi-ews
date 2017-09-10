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

#include "ewsmodifyitemflagsjob.h"

#include "ewsmodifyitemjob.h"
#include "ewsitemhandler.h"

using namespace Akonadi;

EwsModifyItemFlagsJob::EwsModifyItemFlagsJob(EwsClient &client, QObject *parent, const Item::List items,
                                             const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags)
    : EwsJob(parent), mItems(items), mClient(client), mAddedFlags(addedFlags), mRemovedFlags(removedFlags)
{
}

EwsModifyItemFlagsJob::~EwsModifyItemFlagsJob()
{
}

void EwsModifyItemFlagsJob::itemModifyFinished(KJob *job)
{
    if (job->error()) {
        setErrorText(job->errorString());
        emitResult();
        return;
    }

    EwsModifyItemJob *req = qobject_cast<EwsModifyItemJob*>(job);
    if (!req) {
        setErrorText(QStringLiteral("Invalid EwsModifyItemJob job object"));
        emitResult();
        return;
    }

    mResultItems += req->items();
    removeSubjob(job);

    if (subjobs().isEmpty()) {
        Q_ASSERT(mResultItems.size() == mItems.size());
        emitResult();
    }
}

void EwsModifyItemFlagsJob::start()
{
    qDebug() << "started";

    Item::List items[EwsItemTypeUnknown];

    Q_FOREACH(const Item &item, mItems) {
        EwsItemType type = EwsItemHandler::mimeToItemType(item.mimeType());
        if (type == EwsItemTypeUnknown) {
            setErrorText(QStringLiteral("Unknown item type %1 for item %2").arg(item.mimeType()).arg(item.remoteId()));
            emitResult();
            return;
        } else {
            items[type].append(item);
        }
    }

    bool started = false;
    for (int type = 0; type < EwsItemTypeUnknown; type++) {
        if (!items[static_cast<EwsItemType>(type)].isEmpty()) {
            EwsItemHandler *handler = EwsItemHandler::itemHandler(static_cast<EwsItemType>(type));
            EwsModifyItemJob *job = handler->modifyItemJob(mClient, items[type],
                QSet<QByteArray>() << "FLAGS", this);
            connect(job, &EwsModifyItemJob::result, this, &EwsModifyItemFlagsJob::itemModifyFinished);
            addSubjob(job);
            job->start();
            started = true;
        }
    }

    if (!started) {
        setErrorText(QStringLiteral("No items to process"));
        emitResult();
    }
}
