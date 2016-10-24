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
#include <AkonadiCore/AgentManager>
#include <Akonadi/KMime/SpecialMailCollections>
#include <KI18n/KLocalizedString>

#include "ewscreateitemrequest.h"
#include "ewsmoveitemrequest.h"
#include "ewspropertyfield.h"
#include "ewsmailhandler.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidMessageFlags(0x0e07, EwsPropTypeInteger);

EwsCreateMailJob::EwsCreateMailJob(EwsClient& client, const Akonadi::Item &item,
                                   const Akonadi::Collection &collection, EwsTagStore *tagStore,
                                   EwsResource *parent)
    : EwsCreateItemJob(client, item, collection, tagStore, parent), mSend(false)
{
}
EwsCreateMailJob::~EwsCreateMailJob()
{
}

void EwsCreateMailJob::doStart()
{
    if (!mItem.hasPayload<KMime::Message::Ptr>()) {
        setErrorMsg(QStringLiteral("Expected MIME message payload"));
        emitResult();
    }

    EwsCreateItemRequest *req = new EwsCreateItemRequest(mClient, this);

    KMime::Message::Ptr msg = mItem.payload<KMime::Message::Ptr>();
    QByteArray mimeContent = msg->encodedContent(true);
    bool sentItemsCreateWorkaround = false;
    EwsItem item;
    item.setType(EwsItemTypeMessage);
    item.setField(EwsItemFieldMimeContent, mimeContent);
    if (!mSend) {
        /* When creating items using the CreateItem request Exchange will by default mark the  message
         * as draft. Setting the extended property below causes the message to appear normally. */
        item.setProperty(propPidMessageFlags, QStringLiteral("1"));
        const Akonadi::AgentInstance& inst = Akonadi::AgentManager::self()->instance(mCollection.resource());

        /* WORKAROUND: The "Sent Items" folder is a little "special" when it comes to creating items.
         * Unlike other folders when creating items there the creation date/time is always set to the
         * current date/time instead of the value from the MIME Date header. This causes mail that
         * was copied from other folders to appear with the current date/time instead of the original one.
         * To work around this create the item in the "Drafts" folder first and then move it to "Sent Items". */
        if (mCollection == SpecialMailCollections::self()->collection(SpecialMailCollections::SentMail, inst)) {
            qCInfoNC(EWSRES_LOG) << "Move to \"Sent Items\" detected - activating workaround.";
            const Collection &draftColl = SpecialMailCollections::self()->collection(SpecialMailCollections::Drafts, inst);
            req->setSavedFolderId(EwsId(draftColl.remoteId(), draftColl.remoteRevision()));
            sentItemsCreateWorkaround = true;
        } else {
            req->setSavedFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
        }
    }
    // Set flags
    QHash<EwsPropertyField, QVariant> propertyHash = EwsMailHandler::writeFlags(mItem.flags());
    for (auto it = propertyHash.cbegin(); it != propertyHash.cend(); it++) {
        if (!it.value().isNull()) {
            if (it.key().type() == EwsPropertyField::ExtendedField) {
                item.setProperty(it.key(), it.value());
            } else if (it.key().type() == EwsPropertyField::Field) {
                /* TODO: Currently EwsItem distinguishes between regular fields and extended fields
                 * and keeps them in separate lists. Someday it will make more sense to unify them.
                 * Until that the code below needs to manually translate the field names into
                 * EwsItemField enum items.
                 */
                if (it.key().uri() == QStringLiteral("message:IsRead")) {
                    item.setField(EwsItemFieldIsRead, it.value());
                }
            }
        }
    }

    populateCommonProperties(item);

    req->setItems(EwsItem::List() << item);
    req->setMessageDisposition(mSend ? EwsDispSendOnly : EwsDispSaveOnly);
    connect(req, &EwsCreateItemRequest::finished, this,
            sentItemsCreateWorkaround ? &EwsCreateMailJob::mailCreateWorkaroundFinished : &EwsCreateMailJob::mailCreateFinished);
    addSubjob(req);
    req->start();
}

void EwsCreateMailJob::mailCreateFinished(KJob *job)
{
    qDebug() << "mailCreateFinished";
    EwsCreateItemRequest *req = qobject_cast<EwsCreateItemRequest*>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsCreateItemRequest job object"));
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

void EwsCreateMailJob::mailCreateWorkaroundFinished(KJob *job)
{
    qDebug() << "mailCreateWorkaroundFinished";

    EwsCreateItemRequest *req = qobject_cast<EwsCreateItemRequest*>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsCreateItemRequest job object"));
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
        EwsMoveItemRequest *req = new EwsMoveItemRequest(mClient, this);
        req->setItemIds(EwsId::List() << id);
        req->setDestinationFolderId(mCollection.remoteId());
        connect(req, &EwsCreateItemRequest::finished, this, &EwsCreateMailJob::mailMoveWorkaroundFinished);
        addSubjob(req);
        req->start();
    }
    else {
        setErrorMsg(i18n("Failed to create mail item"));
        emitResult();
    }
}

void EwsCreateMailJob::mailMoveWorkaroundFinished(KJob *job)
{
    qDebug() << "mailMoveWorkaroundFinished";

    EwsMoveItemRequest *req = qobject_cast<EwsMoveItemRequest*>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsMoveItemRequest job object"));
        emitResult();
        return;
    }

    if (req->responses().count() != 1) {
        setErrorMsg(QStringLiteral("Invalid number of responses received from server."));
        emitResult();
        return;
    }

    EwsMoveItemRequest::Response resp = req->responses().first();
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


bool EwsCreateMailJob::setSend(bool send)
{
    mSend = send;
    return true;
}
