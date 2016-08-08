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

#ifndef EWSSUBSCRIPTIONMANAGER_H
#define EWSSUBSCRIPTIONMANAGER_H

#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QSet>

#include "ewsid.h"

class EwsClient;
class KJob;
class EwsEventRequestBase;

/**
 *  @brief  Mailbox update subscription manager class
 *
 *  This class is responsible for retrieving update notifications from the Exchange server.
 *
 *  The Exchange server has the ability to incrementally inform the client about changes made to
 *  selected folders in the mailbox. Each update informs about creation, modification or removal
 *  of an item or folder. Additionally Exchange has the ability to notify about free/busy status
 *  updates.
 *
 *  Notifications can be delivered in 3 ways:
 *   - pull (i.e. polling) - the client needs to periodically question the server.
 *   - push - the server issues a callback connection to the client with events (not supported)
 *   - streaming - a combination of pull and push, where the client makes the connection, but the
 *                 server keeps it open for a specified period of time and keeps delivering events
 *                 over this connection (supported since Exchange 2010 SP2).
 *
 *  The responsibility of this class is to retrieve and act upon change events from the Exchange
 *  server. The current implementation is simplified:
 *   - when an item update is received the folder containing the update is asked to synchronise
 *     itself.
 *   - when a folder update is received a full collection tree sync is performed.
 *
 *  The above implementation has a major drawback in that operations performed by the resource
 *  itself are also notified back as update events. This means that when for ex. an item is deleted
 *  it is removed from Akonadi database, but subsequently a delete event is received which will try
 *  to delete an item that has already been deleted from Akonadi.
 *
 *  To reduce such feedback loops the class implements a queued update mechanism. Each time an
 *  operation is performed on the mailbox the resource class is responsible for informing the
 *  subscription manager about it by adding an entry about the performed operation and its subject.
 *  The subscription manager will in turn filter out update events that refer to oprerations that
 *  have already been made.
 */
class EwsSubscriptionManager : public QObject
{
    Q_OBJECT
public:
    EwsSubscriptionManager(EwsClient &client, const EwsId &rootId, QObject *parent);
    virtual ~EwsSubscriptionManager();
    void start();
    void queueUpdate(EwsEventType type, QString id, QString changeKey);
Q_SIGNALS:
    void foldersModified(EwsId::List folders);
    void folderTreeModified();
    void fullSyncRequested();
private Q_SLOTS:
    void subscribeRequestFinished(KJob *job);
    void verifySubFoldersRequestFinished(KJob *job);
    void getEventsRequestFinished(KJob *job);
    void streamingEventsReceived(KJob *job);
    void getEvents();
    void streamingConnectionTimeout();
private:
    void cancelSubscription();
    void setupSubscription();
    void setupSubscriptionReq(const EwsId::List &ids);
    void reset();
    void processEvents(EwsEventRequestBase *req, bool finished);

    struct UpdateItem {
        EwsEventType type;
        QString changeKey;
    };

    EwsClient &mEwsClient;
    QString mSubId;
    QString mWatermark;
    QTimer mPollTimer;
    EwsId mMsgRootId;

    QSet<EwsId> mUpdatedFolderIds;
    bool mFolderTreeChanged;
    bool mStreamingEvents;
    QMultiHash<QString, UpdateItem> mQueuedUpdates;
    QTimer mStreamingTimer;
    EwsEventRequestBase *mEventReq;
};

#endif
