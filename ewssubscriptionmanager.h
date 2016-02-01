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

class EwsSubscriptionManager : public QObject
{
    Q_OBJECT
public:
    EwsSubscriptionManager(EwsClient &client, const EwsId &rootId, QObject *parent);
    virtual ~EwsSubscriptionManager();
    void start();
Q_SIGNALS:
    void foldersModified(EwsId::List folders);
    void folderTreeModified();
    void fullSyncRequested();
private Q_SLOTS:
    void subscribeRequestFinished(KJob *job);
    void getEventsRequestFinished(KJob *job);
    void streamingEventsReceived(KJob *job);
    void getEvents();
private:
    void cancelSubscription();
    void setupSubscription();
    void reset();
    void processEvents(EwsEventRequestBase *req, bool finished);

    EwsClient &mEwsClient;
    QString mSubId;
    QString mWatermark;
    QTimer mPollTimer;
    EwsId mMsgRootId;

    QSet<EwsId> mUpdatedFolderIds;
    bool mFolderTreeChanged;
    bool mStreamingEvents;
};

#endif
