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

#include <QtCore/QDebug>

#include <KI18n/KLocalizedString>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionFetchScope>
#include <Akonadi/KMime/MessageFlags>
#include <KMime/Message>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include "ewsresource.h"
#include "ewsfetchitemsjob.h"
#include "ewsfindfolderrequest.h"
#include "ewseffectiverights.h"
#include "ewsgetitemrequest.h"
#include "ewsupdateitemrequest.h"
#include "ewsmoveitemrequest.h"
#include "ewssubscriptionmanager.h"
#include "configdialog.h"
#include "settings.h"

using namespace Akonadi;

static const EwsPropertyField propPidTagContainerClass(0x3613, EwsPropTypeString);
static const EwsPropertyField propPidFlagStatus(0x1090, EwsPropTypeInteger);

EwsResource::EwsResource(const QString &id)
    : Akonadi::ResourceBase(id)
{
    qDebug() << "EwsResource";
    //setName(i18n("Microsoft Exchange"));
    mEwsClient.setUrl(Settings::self()->baseUrl());

    mRootCollection.setParentCollection(Collection::root());
    mRootCollection.setName(name());
    mRootCollection.setContentMimeTypes(QStringList() << Collection::mimeType() << KMime::Message::mimeType());
    mRootCollection.setRights(Collection::ReadOnly);
    mRootCollection.setRemoteId("root");
    mRootCollection.setRemoteRevision("0");

    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);

    mSubManager.reset(new EwsSubscriptionManager(mEwsClient, this));

    Q_EMIT status(0);
}

EwsResource::~EwsResource()
{
}

void EwsResource::retrieveCollections()
{
    qDebug() << "retrieveCollections";
    EwsFindFolderRequest *req = new EwsFindFolderRequest(mEwsClient, this);
    req->setParentFolderId(EwsDIdMsgFolderRoot);
    EwsFolderShape shape;
    shape << propPidTagContainerClass;
    shape << EwsPropertyField("folder:EffectiveRights");
    req->setFolderShape(shape);
    connect(req, &EwsFindFolderRequest::finished, req,
            [this](KJob *job){findFoldersRequestFinished(qobject_cast<EwsFindFolderRequest*>(job));});
    req->start();
}

void EwsResource::retrieveItems(const Collection &collection)
{
    Q_EMIT status(1, QStringLiteral("Retrieving item list"));
    qDebug() << "retrieveItems";

    EwsFetchItemsJob *job = new EwsFetchItemsJob(collection, mEwsClient,
        mSyncState.value(collection.remoteId()), this);
    connect(job, SIGNAL(finished(KJob*)), SLOT(itemFetchJobFinished(KJob*)));
    connect(job, SIGNAL(status(int,const QString&)), SIGNAL(status(int,const QString&)));
    connect(job, SIGNAL(percent(int)), SIGNAL(percent(int)));
    job->start();
}

bool EwsResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    qDebug() << "retrieveItem";
    EwsGetItemRequest *req = new EwsGetItemRequest(mEwsClient, this);
    EwsId::List ids;
    ids << EwsId(item.remoteId(), item.remoteRevision());
    req->setItemIds(ids);
    EwsItemShape shape(EwsShapeIdOnly);
    shape << EwsPropertyField("item:MimeContent");
    req->setItemShape(shape);
    req->setProperty("item", QVariant::fromValue<Item>(item));
    connect(req, &EwsGetItemRequest::result, req,
            [this](KJob *job){getItemRequestFinished(qobject_cast<EwsGetItemRequest*>(job));});
    req->start();
    return true;
}

void EwsResource::configure(WId windowId)
{
    ConfigDialog dlg(this, windowId);
    if (dlg.exec()) {
        mEwsClient.setUrl(Settings::self()->baseUrl());
        Settings::self()->save();
    }
}

void EwsResource::findFoldersRequestFinished(EwsFindFolderRequest *req)
{
    qDebug() << "findFoldersRequestFinished";

    if (req->error()) {
        qWarning() << "ERROR" << req->errorString();
        cancelTask(req->errorString());
        return;
    }

    qDebug() << "Processing folders";

    Collection::List collections;

    collections.append(mRootCollection);

    Q_FOREACH(const EwsFolder &baseFolder, req->folders()) {
        Collection collection = createFolderCollection(baseFolder);
        collection.setParentCollection(mRootCollection);

        collections.append(collection);

        collections << createChildCollections(baseFolder, collection);
    }

    qDebug() << collections;

    collectionsRetrieved(collections);
}

Collection::List EwsResource::createChildCollections(const EwsFolder &folder, Collection collection)
{
    Collection::List collections;

    Q_FOREACH(const EwsFolder& child, folder.childFolders()) {
        Collection col = createFolderCollection(child);
        col.setParentCollection(collection);
        collections.append(col);

        collections << createChildCollections(child, col);

    }
    return collections;
}

Collection EwsResource::createFolderCollection(const EwsFolder &folder)
{
    Collection collection;
    collection.setName(folder[EwsFolderFieldDisplayName].toString());
    QStringList mimeTypes;
    QString contClass = folder[propPidTagContainerClass].toString();
    mimeTypes.append(Collection::mimeType());
    switch (folder.type()) {
    case EwsFolderTypeCalendar:
        mimeTypes.append(KCalCore::Event::eventMimeType());
        break;
    case EwsFolderTypeContacts:
        mimeTypes.append(KContacts::Addressee::mimeType());
        mimeTypes.append(KContacts::ContactGroup::mimeType());
        break;
    case EwsFolderTypeTasks:
        mimeTypes.append(KCalCore::Todo::todoMimeType());
        break;
    case EwsFolderTypeMail:
        if (contClass == QStringLiteral("IPF.Note") || contClass.isEmpty()) {
            mimeTypes.append(KMime::Message::mimeType());
        }
        break;
    default:
        break;
    }
    collection.setContentMimeTypes(mimeTypes);
    Collection::Rights colRights;
    EwsEffectiveRights ewsRights = folder[EwsFolderFieldEffectiveRights].value<EwsEffectiveRights>();
    if (ewsRights.canDelete()) {
        colRights |= Collection::CanDeleteCollection | Collection::CanDeleteItem;
    }
    if (ewsRights.canModify()) {
        colRights |= Collection::CanChangeCollection | Collection::CanChangeItem;
    }
    if (ewsRights.canCreateContents()) {
        colRights |= Collection::CanCreateItem;
    }
    if (ewsRights.canCreateHierarchy()) {
        colRights |= Collection::CanCreateCollection;
    }
    collection.setRights(colRights);
    EwsId id = folder[EwsFolderFieldFolderId].value<EwsId>();
    collection.setRemoteId(id.id());
    collection.setRemoteRevision(id.changeKey());
    return collection;
}

void EwsResource::itemFetchJobFinished(KJob *job)
{
    EwsFetchItemsJob *fetchJob = qobject_cast<EwsFetchItemsJob*>(job);

    if (!fetchJob) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid job object");
        cancelTask(QStringLiteral("Invalid job object"));
        return;
    }
    if (job->error()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Item fetch error:") << job->errorString();
        if (mSyncState.contains(fetchJob->collection().remoteId())) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Retrying with empty state.");
            // Retry with a clear sync state.
            mSyncState.remove(fetchJob->collection().remoteId());
            retrieveItems(fetchJob->collection());
        }
        else {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Clean sync failed.");
            // No more hope
            cancelTask(job->errorString());
            return;
        }
    }
    else {
        mSyncState[fetchJob->collection().remoteId()] = fetchJob->syncState();
        itemsRetrievedIncremental(fetchJob->changedItems(), fetchJob->deletedItems());
    }
    Q_EMIT status(0);
}

void EwsResource::getItemRequestFinished(EwsGetItemRequest *req)
{
    qDebug() << "getItemRequestFinished";

    if (req->error()) {
        qWarning() << "ERROR" << req->errorString();
        cancelTask(req->errorString());
        return;
    }

    Item item = req->property("item").value<Item>();
    const EwsGetItemRequest::Response &resp = req->responses()[0];
    if (!resp.isSuccess()) {
        qWarning() << QStringLiteral("Item fetch failed!");
        cancelTask(QStringLiteral("Item fetch failed!"));
        return;
    }
    const EwsItem &ewsItem = resp.item();
    QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
    if (mimeContent.isEmpty()) {
        qWarning() << QStringLiteral("MIME content is empty!");
        cancelTask(QStringLiteral("MIME content is empty!"));
        return;
    }

    mimeContent.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    if (ewsItem.isValid()) {
        switch (ewsItem.type()) {
        case EwsItemTypeMessage:
        case EwsItemTypeMeetingMessage:
        case EwsItemTypeMeetingRequest:
        case EwsItemTypeMeetingResponse:
        case EwsItemTypeMeetingCancellation:
        {
            KMime::Message::Ptr msg(new KMime::Message);
            msg->setContent(mimeContent.toLatin1());
            msg->parse();
            qDebug() << msg->head();
            qDebug() << msg->body();
            item.setPayload<KMime::Message::Ptr>(msg);
            qDebug() << item.availablePayloadParts();
            break;
        }
        case EwsItemTypeContact:
            break;
        case EwsItemTypeCalendarItem:
        case EwsItemTypeTask:

            break;
        default:
            // No idea what kind of item it is - skip it.
            break;
        }
        itemRetrieved(item);
    }
}

void EwsResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    qDebug() << "itemChanged" << item.remoteId() << partIdentifiers << item.hasPayload<KCalCore::Event::Ptr>();

    if (item.mimeType() == KMime::Message::mimeType()) {
        mailItemChanged(item, partIdentifiers);
    }
    else {
        cancelTask("Item type not supported for changing");
    }
}

void EwsResource::mailItemChanged(const Item &item, const QSet<QByteArray> &partIdentifiers)
{
    bool doSubmit = false;
    EwsUpdateItemRequest *req = new EwsUpdateItemRequest(mEwsClient, this);
    EwsId itemId(item.remoteId(), item.remoteRevision());

    if (partIdentifiers.contains("FLAGS")) {
        EwsUpdateItemRequest::ItemChange ic(itemId, EwsItemTypeMessage);
        qDebug() << "Item flags" << item.flags();
        bool isRead = item.flags().contains(MessageFlags::Seen);
        EwsUpdateItemRequest::Update *upd =
                        new EwsUpdateItemRequest::SetUpdate(EwsPropertyField(QStringLiteral("message:IsRead")),
                                                            isRead ? QStringLiteral("true") : QStringLiteral("false"));
        ic.addUpdate(upd);
        bool isFlagged = item.flags().contains(MessageFlags::Flagged);
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
        req->setProperty("item", QVariant::fromValue<Item>(item));
        connect(req, SIGNAL(result(KJob*)), SLOT(itemChangeRequestFinished(KJob*)));
        req->start();
    }
    else {
        delete req;
        qDebug() << "Nothing to do for parts" << partIdentifiers;
        changeCommitted(item);
    }
}

void EwsResource::itemChangeRequestFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsUpdateItemRequest *req = qobject_cast<EwsUpdateItemRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid job object"));
        return;
    }

    EwsUpdateItemRequest::Response resp = req->responses().first();
    if (!resp.isSuccess()) {
        cancelTask(QStringLiteral("Item update failed: ") + resp.responseMessage());
        return;
    }

    Item item = job->property("item").value<Item>();
    if (item.isValid()) {
        item.setRemoteRevision(resp.itemId().changeKey());
    }

    changeCommitted(item);
}

void EwsResource::itemsMoved(const Item::List &items, const Collection &sourceCollection,
                             const Collection &destinationCollection)
{
    Q_UNUSED(sourceCollection);

    EwsId::List ids;
    Q_FOREACH(const Item &item, items) {
        EwsId id(item.remoteId(), item.remoteRevision());
        ids.append(id);
    }

    EwsMoveItemRequest *req = new EwsMoveItemRequest(mEwsClient, this);
    req->setItemIds(ids);
    EwsId destId(destinationCollection.remoteId(), QString());
    req->setDestinationFolderId(destId);
    req->setProperty("items", QVariant::fromValue<Item::List>(items));
    connect(req, SIGNAL(result(KJob*)), SLOT(itemMoveRequestFinished(KJob*)));
    req->start();
}

void EwsResource::itemMoveRequestFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    EwsMoveItemRequest *req = qobject_cast<EwsMoveItemRequest*>(job);
    if (!req) {
        cancelTask(QStringLiteral("Invalid job object"));
        return;
    }
    Item::List items = job->property("items").value<Item::List>();

    if (items.count() != req->responses().count()) {
        cancelTask(QStringLiteral("Invalid number of responses received from server."));
        return;
    }

    Item::List::iterator it = items.begin();
    Q_FOREACH(const EwsMoveItemRequest::Response &resp, req->responses()) {
        Item &item = *it;
        if (resp.isSuccess()) {
            if (item.isValid()) {
                item.setRemoteRevision(resp.itemId().changeKey());
            }

        }
        qDebug() << "changeCommitted" << item.remoteId();
        changeCommitted(item);
        it++;
    }
}



AKONADI_RESOURCE_MAIN(EwsResource)
