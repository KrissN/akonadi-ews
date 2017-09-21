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

#include "ewsfetchcalendardetailjob.h"

#include <QTimeZone>

#include <KCalCore/Event>
#include <KCalCore/ICalFormat>
#include <KCalCore/MemoryCalendar>
#include <KDE/KDateTime>
#include <KDE/KSystemTimeZones>
#include <KDE/KTimeZone>

#include "ewsclient_debug.h"
#include "ewsgetitemrequest.h"
#include "ewsitemshape.h"
#include "ewsmailbox.h"
#include "ewsoccurrence.h"

using namespace Akonadi;

EwsFetchCalendarDetailJob::EwsFetchCalendarDetailJob(EwsClient &client, QObject *parent,
        const Collection &collection)
    : EwsFetchItemDetailJob(client, parent, collection)
{
    EwsItemShape shape(EwsShapeIdOnly);
/*    shape << EwsPropertyField(QStringLiteral("calendar:UID"));
    shape << EwsPropertyField(QStringLiteral("item:Subject"));
    shape << EwsPropertyField(QStringLiteral("item:Body"));
    shape << EwsPropertyField(QStringLiteral("calendar:Organizer"));
    shape << EwsPropertyField(QStringLiteral("calendar:RequiredAttendees"));
    shape << EwsPropertyField(QStringLiteral("calendar:OptionalAttendees"));
    shape << EwsPropertyField(QStringLiteral("calendar:Resources"));
    shape << EwsPropertyField(QStringLiteral("calendar:Start"));
    shape << EwsPropertyField(QStringLiteral("calendar:End"));
    shape << EwsPropertyField(QStringLiteral("calendar:IsAllDayEvent"));
    shape << EwsPropertyField(QStringLiteral("calendar:LegacyFreeBusyStatus"));
    shape << EwsPropertyField(QStringLiteral("calendar:AppointmentSequenceNumber"));
    shape << EwsPropertyField(QStringLiteral("calendar:IsRecurring"));
    shape << EwsPropertyField(QStringLiteral("calendar:Recurrence"));
    shape << EwsPropertyField(QStringLiteral("calendar:FirstOccurrence"));
    shape << EwsPropertyField(QStringLiteral("calendar:LastOccurrence"));
    shape << EwsPropertyField(QStringLiteral("calendar:ModifiedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("calendar:DeletedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("calendar:StartTimeZone"));
    shape << EwsPropertyField(QStringLiteral("calendar:EndTimeZone"));
    shape << EwsPropertyField(QStringLiteral("calendar:MyResponseType"));
    shape << EwsPropertyField(QStringLiteral("item:HasAttachments"));
    shape << EwsPropertyField(QStringLiteral("item:Attachments"));*/

//    shape << EwsPropertyField(QStringLiteral("item:Attachments"));
    shape << EwsPropertyField(QStringLiteral("calendar:ModifiedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("calendar:DeletedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("item:Body"));
    shape << EwsPropertyField(QStringLiteral("item:Culture"));
    shape << EwsPropertyField(QStringLiteral("item:MimeContent"));
    shape << EwsPropertyField(QStringLiteral("item:Subject"));
    shape << EwsPropertyField(QStringLiteral("calendar:TimeZone"));
    mRequest->setItemShape(shape);
}


EwsFetchCalendarDetailJob::~EwsFetchCalendarDetailJob()
{
}

void EwsFetchCalendarDetailJob::processItems(const QList<EwsGetItemRequest::Response> &responses)
{
    Item::List::iterator it = mChangedItems.begin();
    KCalCore::ICalFormat format;

    EwsId::List addItems;

    Q_FOREACH (const EwsGetItemRequest::Response &resp, responses) {
        Item &item = *it;

        if (!resp.isSuccess()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch item %1").arg(item.remoteId());
            continue;
        }

        const EwsItem &ewsItem = resp.item();
        QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar(QTimeZone::utc()));
        format.fromString(memcal, mimeContent);
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Found %1 events").arg(memcal->events().count());
        KCalCore::Incidence::Ptr incidence;
        if (memcal->events().count() > 1) {
            Q_FOREACH (KCalCore::Event::Ptr event, memcal->events()) {
                qCDebugNC(EWSRES_LOG) << QString::number(event->recurrence()->recurrenceType(), 16) << event->recurrenceId() << event->recurrenceId().isValid();
                if (!event->recurrenceId().isValid()) {
                    incidence = event;
                }
            }
            EwsOccurrence::List excList = ewsItem[EwsItemFieldModifiedOccurrences].value<EwsOccurrence::List>();
            Q_FOREACH (const EwsOccurrence &exc, excList) {
                addItems.append(exc.itemId());
            }
        } else if (memcal->events().count() == 1) {
            incidence = memcal->events()[0];
        }
        //KCalCore::Incidence::Ptr incidence(format.fromString(mimeContent));

        if (incidence) {
            QString msTz = ewsItem[EwsItemFieldTimeZone].toString();
            QString culture = ewsItem[EwsItemFieldCulture].toString();
            QDateTime dt(incidence->dtStart());
            if (dt.isValid()) {
                incidence->setDtStart(dt);
            }
            if (incidence->type() == KCalCore::Incidence::TypeEvent) {
                KCalCore::Event *event = reinterpret_cast<KCalCore::Event*>(incidence.data());
                dt = event->dtEnd();
                if (dt.isValid()) {
                    event->setDtEnd(dt);
                }
            }
            dt = incidence->recurrenceId();
            if (dt.isValid()) {
                incidence->setRecurrenceId(dt);
            }

            item.setPayload<KCalCore::Incidence::Ptr>(incidence);
        }

        it++;
    }

    if (addItems.isEmpty()) {
        emitResult();
    } else {
        EwsGetItemRequest *req = new EwsGetItemRequest(mClient, this);
        EwsItemShape shape(EwsShapeIdOnly);
//        shape << EwsPropertyField(QStringLiteral("item:Attachments"));
        shape << EwsPropertyField(QStringLiteral("item:Body"));
        shape << EwsPropertyField(QStringLiteral("item:MimeContent"));
        shape << EwsPropertyField(QStringLiteral("calendar:TimeZone"));
        shape << EwsPropertyField(QStringLiteral("item:Culture"));
        req->setItemShape(shape);

        req->setItemIds(addItems);
        connect(req, SIGNAL(result(KJob*)), SLOT(exceptionItemsFetched(KJob*)));
        req->start();
    }
}

void EwsFetchCalendarDetailJob::exceptionItemsFetched(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    EwsGetItemRequest *req = qobject_cast<EwsGetItemRequest*>(job);

    if (!req) {
        setError(1);
        setErrorText(QStringLiteral("Job is not an instance of EwsGetItemRequest"));
        emitResult();
        return;
    }

    KCalCore::ICalFormat format;
    Q_FOREACH (const EwsGetItemRequest::Response &resp, req->responses()) {
        if (!resp.isSuccess()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch item.");
            continue;
        }
        const EwsItem &ewsItem = resp.item();

        Item item(KCalCore::Event::eventMimeType());
        item.setParentCollection(mCollection);
        EwsId id = ewsItem[EwsItemFieldItemId].value<EwsId>();
        item.setRemoteId(id.id());
        item.setRemoteRevision(id.changeKey());

        QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar(QTimeZone::utc()));
        format.fromString(memcal, mimeContent);
        KCalCore::Incidence::Ptr incidence(memcal->events().last());
        incidence->clearRecurrence();

        QString msTz = ewsItem[EwsItemFieldStartTimeZone].toString();
        QString culture = ewsItem[EwsItemFieldCulture].toString();
        QDateTime dt(incidence->dtStart());
        if (dt.isValid()) {
            incidence->setDtStart(dt);
        }
        if (incidence->type() == KCalCore::Incidence::TypeEvent) {
            KCalCore::Event *event = reinterpret_cast<KCalCore::Event*>(incidence.data());
            dt = event->dtEnd();
            if (dt.isValid()) {
                event->setDtEnd(dt);
            }
        }
        dt = incidence->recurrenceId();
        if (dt.isValid()) {
            incidence->setRecurrenceId(dt);
        }

        item.setPayload<KCalCore::Incidence::Ptr>(incidence);

        mChangedItems.append(item);
    }

    emitResult();
}
