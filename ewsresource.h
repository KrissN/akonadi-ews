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

#include "ewsclient.h"
#include "ewsid.h"

class FetchItemState;
class EwsGetItemRequest;
class EwsFindFolderRequest;
class EwsFolder;
class EwsSubscriptionManager;

class EwsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Ews.Resource")
public:
    explicit EwsResource(const QString &id);
    ~EwsResource();

    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) Q_DECL_OVERRIDE;
    virtual void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection,
                            const Akonadi::Collection &destinationCollection) Q_DECL_OVERRIDE;
public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
    Q_SCRIPTABLE void clearSyncState();
    Q_SCRIPTABLE void clearFolderSyncState(QString folderId);
protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
private Q_SLOTS:
    void findFoldersRequestFinished(KJob *job);
    void itemFetchJobFinished(KJob *job);
    void getItemRequestFinished(EwsGetItemRequest *req);
    void itemChangeRequestFinished(KJob *job);
    void itemMoveRequestFinished(KJob *job);
    void delayedInit();
    void foldersModifiedEvent(EwsId::List folders);
    void foldersModifiedCollectionSyncFinished(KJob *job);
    void folderTreeModifiedEvent();
    void fullSyncRequestedEvent();
    void rootFolderFetchFinished(KJob *job);
private:
    void finishItemsFetch(FetchItemState *state);

    void saveState();
    void resetUrl();

    EwsClient mEwsClient;
    Akonadi::Collection mRootCollection;
    QScopedPointer<EwsSubscriptionManager> mSubManager;
    QHash<QString, QString> mSyncState;
    QString mFolderSyncState;
};

#endif
