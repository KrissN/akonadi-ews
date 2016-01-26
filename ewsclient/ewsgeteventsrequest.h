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

#ifndef EWSGETEVENTSREQUEST_H
#define EWSGETEVENTSREQUEST_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QDateTime>

#include "ewsid.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsGetEventsRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Notification;
    class Response;

    class Event
    {
    public:
        typedef QList<Event> List;

        EwsEventType type() const { return mType; };
        const QString &watermark() const { return mWatermark; };
        const QDateTime &timestamp() const { return mTimestamp; };
        const EwsId &itemId() const { return mId; };
        const EwsId &parentFolderId() const { return mParentFolderId; };
        uint unreadCount() const { return mUnreadCount; };
        const EwsId &oldItemId() const { return mOldId; };
        const EwsId &oldParentFolderId() const { return mOldParentFolderId; };
        bool itemIsFolder() const { return mIsFolder; };
    protected:
        Event(QXmlStreamReader &reader);
        bool isValid() { return mType != EwsUnknownEvent; };

        EwsEventType mType;
        QString mWatermark;
        QDateTime mTimestamp;
        EwsId mId;
        EwsId mParentFolderId;
        uint mUnreadCount;
        EwsId mOldId;
        EwsId mOldParentFolderId;
        bool mIsFolder;

        friend class EwsGetEventsRequest::Notification;
    };

    class Notification
    {
    public:
        typedef QList<Notification> List;

        const QString &subscriptionId() const { return mSubscriptionId; };
        const QString &previousWatermark() const { return mWatermark; };
        bool hasMoreEvents() const { return mMoreEvents; };
        const Event::List &events() const { return mEvents; };
    protected:
        Notification(QXmlStreamReader &reader);
        bool isValid() { return !mWatermark.isNull(); };
        static bool eventsReader(QXmlStreamReader &reader, QVariant &val);

        QString mSubscriptionId;
        QString mWatermark;
        bool mMoreEvents;
        Event::List mEvents;

        friend class EwsGetEventsRequest::Response;
    };

    class Response : public EwsRequest::Response
    {
    public:
        const Notification::List &notifications() const { return mNotifications; };
    protected:
        Response(QXmlStreamReader &reader);

        Notification::List mNotifications;

        friend class EwsGetEventsRequest;
    };

    EwsGetEventsRequest(EwsClient &client, QObject *parent);
    virtual ~EwsGetEventsRequest();

    void setSubscriptionId(const QString &id) { mSubscriptionId = id; };
    void setWatermark(const QString &watermark) { mWatermark = watermark; };

    virtual void start();

    const QList<Response> &responses() const { return mResponses; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader);
    bool parseNotificationsResponse(QXmlStreamReader &reader);
private:
    QString mSubscriptionId;
    QString mWatermark;
    QList<Response> mResponses;
};

Q_DECLARE_METATYPE(EwsGetEventsRequest::Event::List)

#endif
