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

#ifndef EWSEVENTREQUESTBASE_H
#define EWSEVENTREQUESTBASE_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QDateTime>

#include "ewsid.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsEventRequestBase : public EwsRequest
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
        bool operator==(const Event &other);
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

        friend class EwsEventRequestBase::Notification;
    };

    class Notification
    {
    public:
        typedef QList<Notification> List;

        const QString &subscriptionId() const { return mSubscriptionId; };
        const QString &previousWatermark() const { return mWatermark; };
        bool hasMoreEvents() const { return mMoreEvents; };
        const Event::List &events() const { return mEvents; };
        bool operator==(const Notification &other);
    protected:
        Notification(QXmlStreamReader &reader);
        bool isValid() { return !mSubscriptionId.isNull(); };
        static bool eventsReader(QXmlStreamReader &reader, QVariant &val);

        QString mSubscriptionId;
        QString mWatermark;
        bool mMoreEvents;
        Event::List mEvents;

        friend class EwsEventRequestBase::Response;
    };

    class Response : public EwsRequest::Response
    {
    public:
        const Notification::List &notifications() const { return mNotifications; };
        bool operator==(const Response &other);
    protected:
        Response(QXmlStreamReader &reader);

        Notification::List mNotifications;

        friend class EwsEventRequestBase;
    };

    virtual ~EwsEventRequestBase();

    void setSubscriptionId(const QString &id) { mSubscriptionId = id; };

    const QList<Response> &responses() const { return mResponses; };
protected:
    EwsEventRequestBase(EwsClient &client, const QString &reqName, QObject *parent);
    virtual bool parseResult(QXmlStreamReader &reader);
    bool parseNotificationsResponse(QXmlStreamReader &reader);

    QString mSubscriptionId;
    QList<Response> mResponses;
    const QString mReqName;
};

Q_DECLARE_METATYPE(EwsEventRequestBase::Event::List)

#endif
