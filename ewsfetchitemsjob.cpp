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

#include "ewsfetchitemsjob.h"

#include <KMime/Message>
#include <KMime/HeaderParsing>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <Akonadi/KMime/MessageFlags>

#include "ewsfinditemrequest.h"
#include "ewsgetitemrequest.h"
#include "ewsclient.h"
#include "ewsmailbox.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

static Q_CONSTEXPR int fetchBatchSize = 20;

/**
 * The fetch items job is processed in two stages.
 *
 * The first stage is to query the list of messages on the remote and local sides. For this purpose
 * an EwsFindItemsRequest is started to retrieve remote items (list of ids only) and an ItemFetchJob
 * is started to fetch local items (from cache only). Both of these jobs are started simultaneously.
 *
 * The second stage begins when both item list query jobs have finished. Both item lists are
 * compared to determine lists of new/changed items and deleted items. The list of new/changed items
 * is then used to perform a second remote request in order to retrieve the details of these items.
 * For e-mail items the second fetch only retrieves the item headers. For other items the full
 * MIME content is fetched.
 */

EwsFetchItemsJob::EwsFetchItemsJob(const Collection &collection, EwsClient &client, QObject *parent)
    : EwsJob(parent), mCollection(collection), mClient(client), mPendingJobs(0)
{
    qCDebugNC(EWSCLIENT_LOG) << mCollection.name();

    qRegisterMetaType<EwsId::List>();
}

EwsFetchItemsJob::~EwsFetchItemsJob()
{
}

void EwsFetchItemsJob::start()
{
    /* Begin stage 1 - query item list from local and remote side. */
    EwsFindItemRequest *findItemsReq = new EwsFindItemRequest(mClient, this);
    findItemsReq->setFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
    EwsItemShape shape(EwsShapeIdOnly);
    findItemsReq->setItemShape(shape);
    connect(findItemsReq, SIGNAL(result(KJob*)), SLOT(remoteItemFetchDone(KJob*)));
    addSubjob(findItemsReq);

    ItemFetchJob *itemJob = new ItemFetchJob(mCollection);
    ItemFetchScope itemScope;
    itemScope.setCacheOnly(true);
    itemScope.fetchFullPayload(false);
    itemJob->setFetchScope(itemScope);
    connect(itemJob, SIGNAL(result(KJob*)), SLOT(localItemFetchDone(KJob*)));
    addSubjob(itemJob);

    mPendingJobs = 2;
    findItemsReq->start();
    itemJob->start();
}

void EwsFetchItemsJob::localItemFetchDone(KJob *job)
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>(job);

    if (!fetchJob) {
        setErrorMsg(QStringLiteral("Invalid item fetch job pointer."));
        doKill();
        emitResult();
    }

    if (!fetchJob->error()) {
        mLocalItems = fetchJob->items();
        mPendingJobs--;
        if (mPendingJobs == 0) {
            compareItemLists();
        }
    }
}

void EwsFetchItemsJob::remoteItemFetchDone(KJob *job)
{
    EwsFindItemRequest *itemReq = qobject_cast<EwsFindItemRequest*>(job);

    if (!itemReq) {
        setErrorMsg(QStringLiteral("Invalid find item request pointer."));
        doKill();
        emitResult();
    }

    if (!itemReq->error()) {
        mRemoteItems = itemReq->items();
        mPendingJobs--;
        if (mPendingJobs == 0) {
            compareItemLists();
        }
    }
}

void EwsFetchItemsJob::compareItemLists()
{
    /* Begin stage 2 - determine list of new/changed items and fetch details about them. */
    QHash<QString, EwsItem> remoteIds;

    Q_FOREACH(const EwsItem &item, mRemoteItems) {
        EwsId id = item[EwsItemFieldItemId].value<EwsId>();
        remoteIds.insert(id.id(), item);
    }

    EwsId::List toFetchMailIds;
    EwsId::List toFetchOtherIds;
    Item::List toFetchMailItems;
    Item::List toFetchOtherItems;

    Q_FOREACH(Item item, mLocalItems) {
        QHash<QString, EwsItem>::iterator it = remoteIds.find(item.remoteId());
        if (it != remoteIds.end()) {
            EwsId id = it.value()[EwsItemFieldItemId].value<EwsId>();
            if (id.changeKey() != item.remoteRevision()) {
                item.clearPayload();
                item.setRemoteRevision(id.changeKey());
                if (item.mimeType() == KMime::Message::mimeType()) {
                    toFetchMailIds.append(id);
                }
                else {
                    toFetchOtherIds.append(id);
                }
            }
            remoteIds.erase(it);
        }
        else {
            mDeletedItems.append(item);
        }
    }

    for (QHash<QString, EwsItem>::const_iterator it = remoteIds.cbegin();
        it != remoteIds.cend(); it++) {
        QString mimeType;
        bool email;
        switch (it->type()) {
        case EwsItemTypeMessage:
        case EwsItemTypeMeetingMessage:
        case EwsItemTypeMeetingRequest:
        case EwsItemTypeMeetingResponse:
        case EwsItemTypeMeetingCancellation:
            mimeType = KMime::Message::mimeType();
            email = true;
            break;
        case EwsItemTypeContact:
            mimeType = KContacts::Addressee::mimeType();
            break;
        case EwsItemTypeCalendarItem:
            mimeType = KCalCore::Event::eventMimeType();
            break;
        case EwsItemTypeTask:
            mimeType = KCalCore::Todo::todoMimeType();
            break;
        default:
            // No idea what kind of item it is - skip it.
            continue;
        }
        Item item(mimeType);
        item.setParentCollection(mCollection);
        EwsId id = it.value()[EwsItemFieldItemId].value<EwsId>();
        item.setRemoteId(id.id());
        item.setRemoteRevision(id.changeKey());
        if (email) {
            toFetchMailIds.append(id);
            toFetchMailItems.append(item);
        }
        else {
            toFetchOtherIds.append(id);
            toFetchOtherItems.append(item);
        }
    }

    if (!toFetchMailIds.isEmpty()) {
        qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Fetching %1 mail items").arg(toFetchMailIds.size());
        int i;
        for (i = 0; i < toFetchMailIds.size(); i += fetchBatchSize) {
            EwsId::List batchList = toFetchMailIds.mid(i, i + fetchBatchSize);
            EwsGetItemRequest *itemReq = new EwsGetItemRequest(mClient, this);
            itemReq->setItemIds(batchList);
            EwsItemShape shape(EwsShapeIdOnly);
            shape << EwsPropertyField("item:Subject");
            shape << EwsPropertyField("item:Importance");
            shape << EwsPropertyField("item:InternetMessageHeaders");
            shape << EwsPropertyField("message:From");
            shape << EwsPropertyField("message:ToRecipients");
            shape << EwsPropertyField("message:CcRecipients");
            shape << EwsPropertyField("message:BccRecipients");
            shape << EwsPropertyField("message:IsRead");
            shape << EwsPropertyField("item:HasAttachments");
            itemReq->setItemShape(shape);
            itemReq->setProperty("itemList",
                QVariant::fromValue<Item::List>(toFetchMailItems.mid(i, i + fetchBatchSize)));
            connect(itemReq, SIGNAL(result(KJob*)), SLOT(mailItemFetchDone(KJob*)));
            mPendingJobs++;
            addSubjob(itemReq);
            itemReq->start();
        }
    }
    if (!toFetchOtherIds.isEmpty()) {
        qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Fetching %1 non-mail items").arg(toFetchOtherIds.size());
        int i;
        for (i = 0; i < toFetchMailIds.size(); i += fetchBatchSize) {
            EwsId::List batchList = toFetchOtherIds.mid(i, i + fetchBatchSize);
            EwsGetItemRequest *itemReq = new EwsGetItemRequest(mClient, this);
            itemReq->setItemIds(batchList);
            EwsItemShape shape(EwsShapeIdOnly);
            shape << EwsPropertyField("item:MimeContent");
            itemReq->setItemShape(shape);
            itemReq->setProperty("itemList",
                QVariant::fromValue<Item::List>(toFetchOtherItems.mid(i, i + fetchBatchSize)));
            connect(itemReq, SIGNAL(result(KJob*)), SLOT(otherItemFetchDone(KJob*)));
            mPendingJobs++;
            addSubjob(itemReq);
            itemReq->start();
        }
    }

    if (toFetchMailIds.isEmpty() && toFetchOtherIds.isEmpty()) {
        // Nothing to fetch - we're done here.
        emitResult();
    }
}

void EwsFetchItemsJob::mailItemFetchDone(KJob *job)
{
    EwsGetItemRequest *itemReq = qobject_cast<EwsGetItemRequest*>(job);

    if (Q_UNLIKELY(!itemReq)) {
        setErrorMsg(QStringLiteral("Invalid get item request pointer."));
        doKill();
        emitResult();
    }

    if (Q_LIKELY(!itemReq->error())) {
        Item::List items = itemReq->property("itemList").value<Item::List>();
        Item::List::iterator it = items.begin();
        Q_ASSERT(items.size() == itemReq->items().size());

        Q_FOREACH(const EwsItem &ewsItem, itemReq->items()) {
            Item item = *it;

            if (!ewsItem.isValid()) {
                qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to fetch item %1").arg(item.remoteId());
                continue;
            }

            KMime::Message::Ptr msg(new KMime::Message);

            // Rebuild the message headers
            QStringList headers;

            // Start with the miscellaneous headers
            QVariant v = ewsItem[EwsItemFieldInternetMessageHeaders];
            if (Q_LIKELY(v.isValid())) {
                EwsItem::HeaderMap origHeaders = v.value<EwsItem::HeaderMap>();
                EwsItem::HeaderMap::const_iterator headIt = origHeaders.cbegin();
                for (; headIt != origHeaders.cend(); headIt++) {
                    QByteArray key = headIt.key().toLatin1();
                    if (headIt.key().compare("Message-ID", Qt::CaseInsensitive) == 0) {
                        qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Found header: %1 %2").arg(headIt.key()).arg(headIt.value());
                    }
                    KMime::Headers::Base *header = KMime::Headers::createHeader(key);
                    if (!header) {
                        header = new KMime::Headers::Generic(headIt.key().toLatin1().constData(), msg.get());
                    }
                    header->fromUnicodeString(headIt.value(), "utf-8");
                    msg->appendHeader(header);
                }
            }

            // Exchange will not return envelope headers (Subject, From, To, etc.) in the above
            // list. Add them explicitly.
            v = ewsItem[EwsItemFieldSubject];
            if (Q_LIKELY(v.isValid())) {
                msg->subject()->fromUnicodeString(v.toString(), "utf-8");
            }

            v = ewsItem[EwsItemFieldFrom];
            if (Q_LIKELY(v.isValid())) {
                EwsMailbox mbox = v.value<EwsMailbox>();
                msg->from()->addAddress(mbox);
            }

            v = ewsItem[EwsItemFieldToRecipients];
            if (Q_LIKELY(v.isValid())) {
                EwsMailbox::List mboxList = v.value<EwsMailbox::List>();
                QStringList addrList;
                Q_FOREACH(const EwsMailbox &mbox, mboxList) {
                    msg->to()->addAddress(mbox);
                }
            }

            v = ewsItem[EwsItemFieldCcRecipients];
            if (Q_LIKELY(v.isValid())) {
                EwsMailbox::List mboxList = v.value<EwsMailbox::List>();
                QStringList addrList;
                Q_FOREACH(const EwsMailbox &mbox, mboxList) {
                    msg->cc()->addAddress(mbox);
                }
            }

            v = ewsItem[EwsItemFieldBccRecipients];
            if (v.isValid()) {
                EwsMailbox::List mboxList = v.value<EwsMailbox::List>();
                QStringList addrList;
                Q_FOREACH(const EwsMailbox &mbox, mboxList) {
                    msg->bcc()->addAddress(mbox);
                }
            }

            msg->assemble();
            //qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Setting payload for msg %1").arg(item.id()) << msg->head();
            item.setPayload(KMime::Message::Ptr(msg));

            v = ewsItem[EwsItemFieldIsRead];
            if (v.isValid()) {
                if (v.toBool()) {
                    item.setFlag(MessageFlags::Seen);
                }
                else {
                    item.clearFlag(MessageFlags::Seen);
                }
            }

            v = ewsItem[EwsItemFieldHasAttachments];
            if (v.isValid()) {
                if (v.toBool()) {
                    item.setFlag(MessageFlags::HasAttachment);
                }
                else {
                    item.clearFlag(MessageFlags::HasAttachment);
                }
            }

            mChangedItems.append(item);

            it++;
        }

        mPendingJobs--;
        if (mPendingJobs == 0) {
            emitResult();
        }
    }

}

void EwsFetchItemsJob::otherItemFetchDone(KJob *job)
{
    EwsGetItemRequest *itemReq = qobject_cast<EwsGetItemRequest*>(job);

    if (!itemReq) {
        setErrorMsg(QStringLiteral("Invalid get item request pointer."));
        doKill();
        emitResult();
    }

    if (!itemReq->error()) {

        // TODO: Implement

        mPendingJobs--;
        if (mPendingJobs == 0) {
            emitResult();
        }
    }

}
