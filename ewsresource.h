/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include <QScopedPointer>

#include <AkonadiAgentBase/ResourceBase>
#include <AkonadiAgentBase/TransportResourceBase>
#include <akonadi_version.h>

#include "ewsclient.h"
#include "ewsfetchitemsjob.h"
#include "ewsid.h"

#include <ewsconfig.h>

class FetchItemState;
class EwsGetItemRequest;
class EwsFindFolderRequest;
class EwsFolder;
class EwsSubscriptionManager;
class EwsTagStore;
class Settings;

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

    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags,
                                  const QSet<Akonadi::Tag> &removedTags) override;
    void tagAdded(const Akonadi::Tag &tag) override;
    void tagChanged(const Akonadi::Tag &tag) override;
    void tagRemoved(const Akonadi::Tag &tag) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                         const Akonadi::Collection &collectionDestination) override;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) override;
    void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags,
                           const QSet<QByteArray> &removedFlags) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection,
                    const Akonadi::Collection &destinationCollection) override;
    void itemsRemoved(const Akonadi::Item::List &items) override;

    void sendItem(const Akonadi::Item &item) override;

    const Akonadi::Collection &rootCollection() const
    {
        return mRootCollection;
    }

    Settings *settings()
    {
        return mSettings.data();
    }
protected:
    void doSetOnline(bool online) override;
public Q_SLOTS:
    void configure(WId windowId) override;
    Q_SCRIPTABLE void clearFolderSyncState(const QString &folderId);
    Q_SCRIPTABLE void clearFolderSyncState();
    Q_SCRIPTABLE void clearFolderTreeSyncState();
protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
    void retrieveTags() override;
private Q_SLOTS:
    void fetchFoldersJobFinished(KJob *job);
    void fetchFoldersIncrJobFinished(KJob *job);
    void itemFetchJobFinished(KJob *job);
    void getItemsRequestFinished(KJob *job);
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
    void connectionError();
    void reloadConfig();
public Q_SLOTS:
    Q_SCRIPTABLE void sendMessage(const QString &id, const QByteArray &content);
Q_SIGNALS:
    Q_SCRIPTABLE void messageSent(const QString &id, const QString &error);
#ifdef HAVE_SEPARATE_MTA_RESOURCE
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
    QScopedPointer<Settings> mSettings;
};

#endif
