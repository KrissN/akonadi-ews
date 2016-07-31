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

#ifndef EWSRESOURCE_H
#define EWSRESOURCE_H

#include <QtCore/QScopedPointer>
#include <AkonadiAgentBase/ResourceBase>
#include <AkonadiAgentBase/TransportResourceBase>

#include "ewsclient.h"
#include "ewsid.h"
#include "ewsfetchitemsjob.h"

#include "config.h"

class FetchItemState;
class EwsGetItemRequest;
class EwsFindFolderRequest;
class EwsFolder;
class EwsSubscriptionManager;
class EwsTagStore;

class EwsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV4,
    public Akonadi::TransportResourceBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Ews.Resource")
public:
    static const QString akonadiEwsPropsetUuid;
    static const EwsPropertyField globalTagsProperty;
    static const EwsPropertyField globalTagsVersionProperty;
    static const EwsPropertyField tagsProperty;
    static const EwsPropertyField flagsProperty;

    explicit EwsResource(const QString &id);
    ~EwsResource();

    virtual void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags,
                                  const QSet<Akonadi::Tag> &removedTags) Q_DECL_OVERRIDE;
    virtual void tagAdded(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;
    virtual void tagChanged(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;
    virtual void tagRemoved(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                 const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;
    virtual void collectionChanged(const Akonadi::Collection &collection,
                                   const QSet<QByteArray> &changedAttributes) Q_DECL_OVERRIDE;
    virtual void collectionChanged(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) Q_DECL_OVERRIDE;
    virtual void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags,
                                  const QSet<QByteArray> &removedFlags) Q_DECL_OVERRIDE;
    virtual void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection,
                            const Akonadi::Collection &destinationCollection) Q_DECL_OVERRIDE;
    virtual void itemsRemoved(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;

    virtual void sendItem(const Akonadi::Item &item) Q_DECL_OVERRIDE;

    bool requestPassword(QString &password, bool ask);
    void setPassword(const QString &password);

    const Akonadi::Collection &rootCollection() const { return mRootCollection; };
protected:
    void doSetOnline(bool online) Q_DECL_OVERRIDE;
public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
    Q_SCRIPTABLE void clearSyncState();
    Q_SCRIPTABLE void clearFolderSyncState(QString folderId);
protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void retrieveTags() Q_DECL_OVERRIDE;
private Q_SLOTS:
    void findFoldersRequestFinished(KJob *job);
    void itemFetchJobFinished(KJob *job);
    void getItemRequestFinished(EwsGetItemRequest *req);
    void itemChangeRequestFinished(KJob *job);
    void itemModifyFlagsRequestFinished(KJob *job);
    void itemMoveRequestFinished(KJob *job);
    void itemDeleteRequestFinished(KJob *job);
    void itemCreateRequestFinished(KJob *job);
    void itemSendRequestFinished(KJob *job);
    void folderCreateRequestFinished(KJob *job);
    void folderMoveRequestFinished(KJob *job);
    void folderUpdateRequestFinished(KJob *job);
    void folderDeleteRequestFinished(KJob *job);
    void delayedInit();
    void foldersModifiedEvent(EwsId::List folders);
    void foldersModifiedCollectionSyncFinished(KJob *job);
    void folderTreeModifiedEvent();
    void fullSyncRequestedEvent();
    void rootFolderFetchFinished(KJob *job);
    void specialFoldersFetchFinished(KJob *job);
    void itemsTagChangeFinished(KJob *job);
    void globalTagChangeFinished(KJob *job);
    void globalTagsRetrievalFinished(KJob *job);
    void adjustInboxRemoteIdFetchFinished(KJob *job);
    void rootCollectionFetched(KJob *job);
#ifdef HAVE_SEPARATE_MTA_RESOURCE
public Q_SLOTS:
    Q_SCRIPTABLE void sendMessage(QString id, QByteArray content);
Q_SIGNALS:
    Q_SCRIPTABLE void messageSent(QString id, QString error);
private Q_SLOTS:
    void messageSendRequestFinished(KJob *job);
#endif

private:
    void finishItemsFetch(FetchItemState *state);
    void fetchSpecialFolders();
    void specialFoldersCollectionsRetrieved(const Akonadi::Collection::List &folders);

    void saveState();
    void resetUrl();

    void doRetrieveCollections();

    int reconnectTimeout();

    EwsClient mEwsClient;
    Akonadi::Collection mRootCollection;
    QScopedPointer<EwsSubscriptionManager> mSubManager;
    QHash<QString, QString> mSyncState;
    QString mFolderSyncState;
    QHash<QString, EwsId::List> mItemsToCheck;
    QHash<QString, EwsFetchItemsJob::QueuedUpdateList> mQueuedUpdates;
    QString mPassword;
    bool mTagsRetrieved;
    int mReconnectTimeout;
    EwsTagStore *mTagStore;
    QHash<QString, QString> mFolderTreeCache;
    QHash<QString, Akonadi::Collection::Id> mFolderIdCache;
};

#endif
