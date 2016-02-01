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

#include "ewssubscriptionmanager.h"
#include "ewssubscriberequest.h"
#include "ewsunsubscriberequest.h"
#include "ewsgeteventsrequest.h"
#include "ewsgetstreamingeventsrequest.h"
#include "ewsgetfolderrequest.h"
#include "ewsclient_debug.h"

// TODO: Allow customisation
static Q_CONSTEXPR uint pollInterval = 10; /* seconds */

static Q_CONSTEXPR uint streamingTimeout = 30; /* minutes */

EwsSubscriptionManager::EwsSubscriptionManager(EwsClient &client, const EwsId &rootId, QObject *parent)
    : QObject(parent), mEwsClient(client), mPollTimer(this), mMsgRootId(rootId), mFolderTreeChanged(false)
{
    mStreamingEvents = mEwsClient.serverVersion().supports(EwsServerVersion::StreamingSubscription);
}

EwsSubscriptionManager::~EwsSubscriptionManager()
{
    cancelSubscription();
}

void EwsSubscriptionManager::start()
{
    // Set-up change notification subscription
    setupSubscription();

    if (!mStreamingEvents) {
        mStreamingEvents = false;
        mPollTimer.setInterval(pollInterval * 1000);
        mPollTimer.setSingleShot(false);
        connect(&mPollTimer, &QTimer::timeout, this, &EwsSubscriptionManager::getEvents);
    }
}

void EwsSubscriptionManager::cancelSubscription()
{
    if (!mSubId.isNull()) {
        EwsUnsubscribeRequest *req = new EwsUnsubscribeRequest(mEwsClient, this);
        req->setSubscriptionId(mSubId);
        req->exec();
    }
}

void EwsSubscriptionManager::setupSubscription()
{
    EwsSubscribeRequest *req = new EwsSubscribeRequest(mEwsClient, this);
    //req->setAllFolders(true);
    QList<EwsEventType> events;
    events << EwsNewMailEvent;
    events << EwsMovedEvent;
    events << EwsCopiedEvent;
    events << EwsModifiedEvent;
    events << EwsDeletedEvent;
    events << EwsCreatedEvent;
    req->setEventTypes(events);
    if (mStreamingEvents) {
        req->setType(EwsSubscribeRequest::StreamingSubscription);
    }
    else {
        req->setType(EwsSubscribeRequest::PullSubscription);
    }
    EwsId::List ids;
    ids << EwsId(EwsDIdInbox);
    req->setFolderIds(ids);
    req->setAllFolders(false);
    connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::subscribeRequestFinished);
    req->start();
}

void EwsSubscriptionManager::reset()
{
    mPollTimer.stop();
    cancelSubscription();
    setupSubscription();
}

void EwsSubscriptionManager::subscribeRequestFinished(KJob *job)
{
    if (!job->error()) {
        EwsSubscribeRequest *req = qobject_cast<EwsSubscribeRequest*>(job);
        if (req) {
            mSubId = req->response().subscriptionId();
            if (mStreamingEvents) {
                getEvents();
            }
            else {
                mWatermark = req->response().watermark();
                getEvents();
                mPollTimer.start();
            }
        }
    }
}

void EwsSubscriptionManager::getEvents()
{
    if (mStreamingEvents) {
        EwsGetStreamingEventsRequest *req = new EwsGetStreamingEventsRequest(mEwsClient, this);
        req->setSubscriptionId(mSubId);
        req->setTimeout(streamingTimeout);
        connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::getEventsRequestFinished);
        connect(req, &EwsGetStreamingEventsRequest::eventsReceived, this,
                &EwsSubscriptionManager::streamingEventsReceived);
        req->start();
    }
    else {
        EwsGetEventsRequest *req = new EwsGetEventsRequest(mEwsClient, this);
        req->setSubscriptionId(mSubId);
        req->setWatermark(mWatermark);
        connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::getEventsRequestFinished);
        req->start();
    }
}

void EwsSubscriptionManager::getEventsRequestFinished(KJob *job)
{
    EwsEventRequestBase *req = qobject_cast<EwsEventRequestBase*>(job);
    if (!req) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid job object.");
        reset();
        return;
    }

    if (!job->error()) {
        processEvents(req, true);
        if (mStreamingEvents) {
            getEvents();
        }
    }
}

void EwsSubscriptionManager::streamingEventsReceived(KJob *job)
{
    EwsEventRequestBase *req = qobject_cast<EwsEventRequestBase*>(job);
    if (!req) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid job object.");
        reset();
        return;
    }

    if (!job->error()) {
        processEvents(req, false);
    }
}

void EwsSubscriptionManager::processEvents(EwsEventRequestBase *req, bool finished)
{
    bool moreEvents = false;

    Q_FOREACH(const EwsGetEventsRequest::Response &resp, req->responses()) {
        Q_FOREACH(const EwsGetEventsRequest::Notification &nfy, resp.notifications()) {
            Q_FOREACH(const EwsGetEventsRequest::Event &event, nfy.events()) {
                mWatermark = event.watermark();
                switch (event.type()) {
                case EwsCopiedEvent:
                case EwsMovedEvent:
                    if (!event.itemIsFolder()) {
                        mUpdatedFolderIds.insert(event.oldParentFolderId());
                    }
                    /* no break */
                case EwsCreatedEvent:
                case EwsDeletedEvent:
                case EwsModifiedEvent:
                case EwsNewMailEvent:
                    if (event.itemIsFolder()) {
                        mFolderTreeChanged = true;
                    }
                    else {
                        mUpdatedFolderIds.insert(event.parentFolderId());
                    }
                    break;
                case EwsStatusEvent:
                    // Do nothing
                    break;
                default:
                    break;
                }
            }
            if (nfy.hasMoreEvents()) {
                moreEvents = true;
            }
        }
        if (mStreamingEvents) {
            EwsGetStreamingEventsRequest *req2 = qobject_cast<EwsGetStreamingEventsRequest*>(req);
            if (req2) {
                req2->eventsProcessed(resp);
            }
        }
    }

    if (moreEvents && finished) {
        getEvents();
    }
    else {
        if (mFolderTreeChanged) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found modified folder tree");
            Q_EMIT folderTreeModified();
            mFolderTreeChanged = false;
        }
        if (!mUpdatedFolderIds.isEmpty()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found %1 modified folders")
                            .arg(mUpdatedFolderIds.size());
            Q_EMIT foldersModified(mUpdatedFolderIds.toList());
            mUpdatedFolderIds.clear();
        }
    }
}
