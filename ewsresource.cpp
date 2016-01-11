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
#include <KMime/Message>
#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include "ewsresource.h"
#include "ewsfetchitemsjob.h"
#include "ewsfindfolderrequest.h"
#include "ewsgetitemrequest.h"
#include "configdialog.h"
#include "settings.h"

using namespace Akonadi;

static const EwsPropertyField propPidTagContainerClass(0x3613, EwsPropTypeString);
static const EwsPropertyField propItemSubject("item:Subject");

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
    req->setFolderShape(shape);
    connect(req, &EwsFindFolderRequest::finished, req,
            [this](KJob *job){findFoldersRequestFinished(qobject_cast<EwsFindFolderRequest*>(job));});
    req->start();
}

void EwsResource::retrieveItems(const Collection &collection)
{
    Q_EMIT status(1, QStringLiteral("Retrieving item list"));
    qDebug() << "retrieveItems";

    EwsFetchItemsJob *job = new EwsFetchItemsJob(collection, mEwsClient, this);
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
    collection.setRights(Collection::ReadOnly);
    EwsId id = folder[EwsFolderFieldFolderId].value<EwsId>();
    collection.setRemoteId(id.id());
    collection.setRemoteRevision(id.changeKey());
    return collection;
}

void EwsResource::itemFetchJobFinished(KJob *job)
{
    EwsFetchItemsJob *fetchJob = qobject_cast<EwsFetchItemsJob*>(job);

    if (!fetchJob || job->error()) {
        qWarning() << "ERROR" << job->errorString();
        cancelTask(job->errorString());
        return;
    }
    else {
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
    EwsItem ewsItem = req->items()[0];
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

AKONADI_RESOURCE_MAIN(EwsResource)
