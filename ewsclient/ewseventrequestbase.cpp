/*
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

#include "ewseventrequestbase.h"
#include "ewsxml.h"
#include "ewsclient_debug.h"

enum NotificationElementType {
    SubscriptionId,
    PreviousWatermark,
    MoreEvents,
    Events
};
typedef EwsXml<NotificationElementType> NotificationReader;

enum EventElementType {
    Watermark,
    Timestamp,
    ItemId,
    FolderId,
    ParentFolderId,
    OldItemId,
    OldFolderId,
    OldParentFolderId,
    UnreadCount
};
typedef EwsXml<EventElementType> EventReader;

EwsEventRequestBase::EwsEventRequestBase(EwsClient &client, const QString &reqName, QObject *parent)
    : EwsRequest(client, parent), mReqName(reqName)
{
    qRegisterMetaType<EwsEventRequestBase::Event::List>();
}

EwsEventRequestBase::~EwsEventRequestBase()
{
}

bool EwsEventRequestBase::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, mReqName,
                                [this](QXmlStreamReader &reader) {return parseNotificationsResponse(reader);});
}

bool EwsEventRequestBase::parseNotificationsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            uint numEv = 0;
            Q_FOREACH (const Notification &nfy, resp.notifications()) {
                numEv += nfy.events().size();
            }
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got %1 response (%2 notifications, %3 events)")
                                          .arg(mReqName).arg(resp.notifications().size()).arg(numEv);
        } else {
            qCDebug(EWSRES_REQUEST_LOG) << QStringLiteral("Got %1 response - %2")
                                        .arg(mReqName).arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsEventRequestBase::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(QStringLiteral("Unexpected namespace in %1 element: %2")
                        .arg(QStringLiteral("ResponseMessage")).arg(reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == QStringLiteral("Notification")) {
            Notification nfy(reader);
            if (!nfy.isValid()) {
                setErrorMsg(QStringLiteral("Failed to process notification."));
                reader.skipCurrentElement();
                return;
            }
            mNotifications.append(nfy);
        } else if (reader.name() == QStringLiteral("Notifications")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QStringLiteral("Notification")) {
                    Notification nfy(reader);
                    if (!nfy.isValid()) {
                        setErrorMsg(QStringLiteral("Failed to process notification."));
                        reader.skipCurrentElement();
                        return;
                    }
                    mNotifications.append(nfy);
                } else {
                    setErrorMsg(QStringLiteral("Failed to read EWS request - expected Notification inside Notifications"));
                }
            }
        } else if (reader.name() == QStringLiteral("ConnectionStatus")) {
            reader.skipCurrentElement();
        } else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element '%1'")
                        .arg(reader.name().toString()));
            return;
        }
    }
}

EwsEventRequestBase::Notification::Notification(QXmlStreamReader &reader)
{
    static const QVector<NotificationReader::Item> items = {
        {SubscriptionId, QStringLiteral("SubscriptionId"), &ewsXmlTextReader},
        {PreviousWatermark, QStringLiteral("PreviousWatermark"), &ewsXmlTextReader},
        {MoreEvents, QStringLiteral("MoreEvents"), &ewsXmlBoolReader},
        {Events, QStringLiteral("CopiedEvent"), &eventsReader},
        {Events, QStringLiteral("CreatedEvent"), &eventsReader},
        {Events, QStringLiteral("DeletedEvent"), &eventsReader},
        {Events, QStringLiteral("ModifiedEvent"), &eventsReader},
        {Events, QStringLiteral("MovedEvent"), &eventsReader},
        {Events, QStringLiteral("NewMailEvent"), &eventsReader},
        {Events, QStringLiteral("FreeBusyChangeEvent"), &eventsReader},
        {Events, QStringLiteral("StatusEvent"), &eventsReader}
    };
    static const NotificationReader staticReader(items);

    NotificationReader ewsreader(staticReader);

    if (!ewsreader.readItems(reader, ewsTypeNsUri)) {
        return;
    }

    QHash<NotificationElementType, QVariant> values = ewsreader.values();

    mSubscriptionId = values[SubscriptionId].toString();
    mWatermark = values[PreviousWatermark].toString();
    mMoreEvents = values[MoreEvents].toBool();
    mEvents = values[Events].value<Event::List>();
}

bool EwsEventRequestBase::Notification::eventsReader(QXmlStreamReader &reader, QVariant &val)
{
    Event::List events = val.value<Event::List>();
    QString elmName(reader.name().toString());

    Event event(reader);
    if (!event.isValid()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element").arg(elmName);
        return false;
    }

    events.append(event);

    val = QVariant::fromValue<Event::List>(events);
    return true;
}

EwsEventRequestBase::Event::Event(QXmlStreamReader &reader)
    : mType(EwsUnknownEvent)
{
    static const QVector<EventReader::Item> items = {
        {Watermark, QStringLiteral("Watermark"), &ewsXmlTextReader},
        {Timestamp, QStringLiteral("TimeStamp"), &ewsXmlDateTimeReader},
        {FolderId, QStringLiteral("FolderId"), &ewsXmlIdReader},
        {ItemId, QStringLiteral("ItemId"), &ewsXmlIdReader},
        {ParentFolderId, QStringLiteral("ParentFolderId"), &ewsXmlIdReader},
        {OldFolderId, QStringLiteral("OldFolderId"), &ewsXmlIdReader},
        {OldItemId, QStringLiteral("OldItemId"), &ewsXmlIdReader},
        {OldParentFolderId, QStringLiteral("OldParentFolderId"), &ewsXmlIdReader},
        {UnreadCount, QStringLiteral("UnreadCount"), &ewsXmlUIntReader},
    };
    static const EventReader staticReader(items);

    EventReader ewsreader(staticReader);

    QStringRef elmName = reader.name();
    if (elmName == QStringLiteral("CopiedEvent")) {
        mType = EwsCopiedEvent;
    } else if (elmName == QStringLiteral("CreatedEvent")) {
        mType = EwsCreatedEvent;
    } else if (elmName == QStringLiteral("DeletedEvent")) {
        mType = EwsDeletedEvent;
    } else if (elmName == QStringLiteral("ModifiedEvent")) {
        mType = EwsModifiedEvent;
    } else if (elmName == QStringLiteral("MovedEvent")) {
        mType = EwsMovedEvent;
    } else if (elmName == QStringLiteral("NewMailEvent")) {
        mType = EwsNewMailEvent;
    } else if (elmName == QStringLiteral("StatusEvent")) {
        mType = EwsStatusEvent;
    } else if (elmName == QStringLiteral("FreeBusyChangedEvent")) {
        mType = EwsFreeBusyChangedEvent;
    } else {
        qCWarning(EWSRES_LOG) << QStringLiteral("Unknown notification event type: %1")
                              .arg(elmName.toString());
        return;
    }

    if (!ewsreader.readItems(reader, ewsTypeNsUri)) {
        mType = EwsUnknownEvent;
        return;
    }

    QHash<EventElementType, QVariant> values = ewsreader.values();

    mWatermark = values[Watermark].toString();
    mTimestamp = values[Timestamp].value<QDateTime>();
    if (values.contains(ItemId)) {
        mId = values[ItemId].value<EwsId>();
        mOldId = values[OldItemId].value<EwsId>();
        mIsFolder = false;
    } else {
        mId = values[FolderId].value<EwsId>();
        mOldId = values[OldFolderId].value<EwsId>();
        mIsFolder = true;
    }
    mParentFolderId = values[ParentFolderId].value<EwsId>();
    mOldParentFolderId = values[OldParentFolderId].value<EwsId>();
    mUnreadCount = values[UnreadCount].toUInt();

    if (mType == EwsStatusEvent) {
        qCDebugNCS(EWSRES_LOG) << QStringLiteral(" %1").arg(elmName.toString());
    } else {
        qCDebugNCS(EWSRES_LOG) << QStringLiteral(" %1, %2, parent: ").arg(elmName.toString()).arg(mIsFolder ? 'F' : 'I')
                               << mParentFolderId << QStringLiteral(", id: ") << mId;
    }
}

bool EwsEventRequestBase::Response::operator==(const Response &other) const
{
    return mNotifications == other.mNotifications;
}

bool EwsEventRequestBase::Notification::operator==(const Notification &other) const
{
    return (mSubscriptionId == other.mSubscriptionId) && (mWatermark == other.mWatermark) &&
           (mMoreEvents == other.mMoreEvents) && (mEvents == other.mEvents);
}

bool EwsEventRequestBase::Event::operator==(const Event &other) const
{
    return (mType == other.mType) && (mWatermark == other.mWatermark) &&
           (mTimestamp == other.mTimestamp) && (mId == other.mId) &&
           (mParentFolderId == other.mParentFolderId) && (mUnreadCount == other.mUnreadCount) &&
           (mOldId == other.mOldId) && (mOldParentFolderId == other.mOldParentFolderId) &&
           (mIsFolder == other.mIsFolder);
}
