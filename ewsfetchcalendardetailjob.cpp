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

#include "ewsfetchcalendardetailjob.h"

#include <QtCore/QTimeZone>
#include <KCalCore/Event>
#include <KDE/KDateTime>
#include <KDE/KTimeZone>
#include <KDE/KSystemTimeZones>
#include <KCalCore/ICalFormat>
#include <KCalCore/MemoryCalendar>

#include "ewsitemshape.h"
#include "ewsgetitemrequest.h"
#include "ewsoccurrence.h"
#include "ewsmailbox.h"
#include "ewsclient_debug.h"

using namespace Akonadi;

EwsFetchCalendarDetailJob::EwsFetchCalendarDetailJob(EwsClient &client, QObject *parent,
                                                     const Collection &collection)
    : EwsFetchItemDetailJob(client, parent, collection)
{
    EwsItemShape shape(EwsShapeIdOnly);
/*    shape << EwsPropertyField("calendar:UID");
    shape << EwsPropertyField("item:Subject");
    shape << EwsPropertyField("item:Body");
    shape << EwsPropertyField("calendar:Organizer");
    shape << EwsPropertyField("calendar:RequiredAttendees");
    shape << EwsPropertyField("calendar:OptionalAttendees");
    shape << EwsPropertyField("calendar:Resources");
    shape << EwsPropertyField("calendar:Start");
    shape << EwsPropertyField("calendar:End");
    shape << EwsPropertyField("calendar:IsAllDayEvent");
    shape << EwsPropertyField("calendar:LegacyFreeBusyStatus");
    shape << EwsPropertyField("calendar:AppointmentSequenceNumber");
    shape << EwsPropertyField("calendar:IsRecurring");
    shape << EwsPropertyField("calendar:Recurrence");
    shape << EwsPropertyField("calendar:FirstOccurrence");
    shape << EwsPropertyField("calendar:LastOccurrence");
    shape << EwsPropertyField("calendar:ModifiedOccurrences");
    shape << EwsPropertyField("calendar:DeletedOccurrences");
    shape << EwsPropertyField("calendar:StartTimeZone");
    shape << EwsPropertyField("calendar:EndTimeZone");
    shape << EwsPropertyField("calendar:MyResponseType");
    shape << EwsPropertyField("item:HasAttachments");
    shape << EwsPropertyField("item:Attachments");*/

    shape << EwsPropertyField("item:Attachments");
    shape << EwsPropertyField("calendar:ModifiedOccurrences");
    shape << EwsPropertyField("calendar:DeletedOccurrences");
    shape << EwsPropertyField("item:Body");
    shape << EwsPropertyField("item:Culture");
    shape << EwsPropertyField("item:MimeContent");
    shape << EwsPropertyField("item:Subject");
    shape << EwsPropertyField("calendar:StartTimeZone");
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

    Q_FOREACH(const EwsGetItemRequest::Response &resp, responses) {
        Item &item = *it;

        qDebug() << item.remoteId();

        if (!resp.isSuccess()) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to fetch item %1").arg(item.remoteId());
            continue;
        }

        const EwsItem &ewsItem = resp.item();
        QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar("GMT"));
        format.fromString(memcal, mimeContent);
        qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Found %1 events").arg(memcal->events().count());
        KCalCore::Incidence::Ptr incidence;
        if (memcal->events().count() > 1) {
            Q_FOREACH(KCalCore::Event::Ptr event, memcal->events()) {
                qCDebugNC(EWSCLIENT_LOG) << QString::number(event->recurrence()->recurrenceType(), 16) << event->recurrenceId().dateTime() << event->recurrenceId().isValid();
                if (!event->recurrenceId().isValid()) {
                    incidence = event;
                }
            }
            EwsOccurrence::List excList = ewsItem[EwsItemFieldModifiedOccurrences].value<EwsOccurrence::List>();
            Q_FOREACH(const EwsOccurrence &exc, excList) {
                addItems.append(exc.itemId());
            }
        }
        else if (memcal->events().count() == 1) {
            incidence = memcal->events()[0];
        }
        //KCalCore::Incidence::Ptr incidence(format.fromString(mimeContent));

        if (incidence) {
            QString msTz = ewsItem[EwsItemFieldStartTimeZone].toString();
            QString culture = ewsItem[EwsItemFieldCulture].toString();
            KDateTime dt(incidence->dtStart());
            convertTimezone(dt, msTz, culture);
            if (dt.isValid()) {
                incidence->setDtStart(dt);
            }
            if (incidence->type() == KCalCore::Incidence::TypeEvent) {
                KCalCore::Event *event = reinterpret_cast<KCalCore::Event*>(incidence.data());
                dt = event->dtEnd();
                convertTimezone(dt, msTz, culture);
                if (dt.isValid()) {
                    event->setDtEnd(dt);
                }
            }
            dt = incidence->recurrenceId();
            convertTimezone(dt, msTz, culture);
            if (dt.isValid()) {
                incidence->setRecurrenceId(dt);
            }

            item.setPayload<KCalCore::Incidence::Ptr>(incidence);
        }

        it++;
    }

    if (addItems.isEmpty()) {
        qDebug() << "done";

        emitResult();
    }
    else {
        EwsGetItemRequest *req = new EwsGetItemRequest(mClient, this);
        EwsItemShape shape(EwsShapeIdOnly);
        shape << EwsPropertyField("item:Attachments");
        shape << EwsPropertyField("item:Body");
        shape << EwsPropertyField("item:MimeContent");
        shape << EwsPropertyField("calendar:StartTimeZone");
        shape << EwsPropertyField("item:Culture");
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
    Q_FOREACH(const EwsGetItemRequest::Response& resp, req->responses()) {
        if (!resp.isSuccess()) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to fetch item.");
            continue;
        }
        const EwsItem &ewsItem = resp.item();

        Item item(KCalCore::Event::eventMimeType());
        item.setParentCollection(mCollection);
        EwsId id = ewsItem[EwsItemFieldItemId].value<EwsId>();
        item.setRemoteId(id.id());
        item.setRemoteRevision(id.changeKey());

        QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar("GMT"));
        format.fromString(memcal, mimeContent);
        KCalCore::Incidence::Ptr incidence(memcal->events().last());
        incidence->clearRecurrence();

        QString msTz = ewsItem[EwsItemFieldStartTimeZone].toString();
        QString culture = ewsItem[EwsItemFieldCulture].toString();
        KDateTime dt(incidence->dtStart());
        convertTimezone(dt, msTz, culture);
        if (dt.isValid()) {
            incidence->setDtStart(dt);
        }
        if (incidence->type() == KCalCore::Incidence::TypeEvent) {
            KCalCore::Event *event = reinterpret_cast<KCalCore::Event*>(incidence.data());
            dt = event->dtEnd();
            convertTimezone(dt, msTz, culture);
            if (dt.isValid()) {
                event->setDtEnd(dt);
            }
        }
        dt = incidence->recurrenceId();
        convertTimezone(dt, msTz, culture);
        if (dt.isValid()) {
            incidence->setRecurrenceId(dt);
        }

        item.setPayload<KCalCore::Incidence::Ptr>(incidence);

        mChangedItems.append(item);
    }

    emitResult();
}

EwsFetchItemDetailJob *EwsFetchCalendarDetailJob::factory(EwsClient &client, QObject *parent,
                                                          const Akonadi::Collection &collection)
{
    return new EwsFetchCalendarDetailJob(client, parent, collection);
}

/*
 *  This method does its best to convert the timezone found in the EWS calendar event to a IANA
 *  timezone. This is a cumbersome process as there is no guarantee that the
 */
/*KTimeZone EwsFetchCalendarDetailJob::getTimezone(QString msTimezone, QString culture)
{
    return KTimeZone();
}*/

/* This function is a lousy workaround for Exchange returning Windows timezone names instead of
 * IANA ones.
 */
void EwsFetchCalendarDetailJob::convertTimezone(KDateTime &currentTime, QString msTimezone, QString culture)
{
    KDateTime resultDt;

    if (!QTimeZone::isTimeZoneIdAvailable(currentTime.timeZone().name().toLatin1())) {
        // The event uses an incorrect IANA timezone, so this will most likely be a
        // Windows timezone name. Attempt to do the conversion.

        QString ianaTz;
        // Special case:
        if (msTimezone == QStringLiteral("tzone://Microsoft/Utc")) {
            ianaTz = QStringLiteral("UTC");
        }
        else {
            QLocale locale(culture);
            if (msTimezone.isEmpty()) {
                msTimezone = currentTime.timeZone().name();
            }
            qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Time zone '%1' not found on IANA list. Trying to convert with country '%2'")
                            .arg(currentTime.timeZone().name()).arg(QLocale::countryToString(locale.country()));
            qCDebugNC(EWSCLIENT_LOG) << "MSTZ: " << msTimezone;
            ianaTz = QTimeZone::windowsIdToDefaultIanaId(msTimezone.toLatin1(), locale.country());
            if (ianaTz.isEmpty()) {
                // Give it one more try with the default country.
                ianaTz = QTimeZone::windowsIdToDefaultIanaId(msTimezone.toLatin1());
                if (ianaTz.isEmpty()) {
                    // Give it two more tries with the timezone name from the event.
                    ianaTz = QTimeZone::windowsIdToDefaultIanaId(currentTime.timeZone().name().toLatin1(), locale.country());
                    if (ianaTz.isEmpty()) {
                        // Give it one more try with the default country.
                        ianaTz = QTimeZone::windowsIdToDefaultIanaId(currentTime.timeZone().name().toLatin1());
                        qCWarningNC(EWSCLIENT_LOG)
                            << QStringLiteral("Failed to convert time zone '%1' or '%2' to IANA id").arg(msTimezone).arg(currentTime.timeZone().name());
                    }
                }
            }
        }
        if (!ianaTz.isEmpty()) {
            qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("Found IANA time zone '%1'").arg(ianaTz);
            KTimeZone newTz = KSystemTimeZones::timeZones()->zone(ianaTz);
            resultDt = KDateTime(currentTime.dateTime(), newTz);
            qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("New timezone: '%1'").arg(resultDt.timeZone().name());
        }
    }

    currentTime = resultDt;
}

EWS_DECLARE_FETCH_ITEM_DETAIL_JOB(EwsFetchCalendarDetailJob, EwsItemTypeCalendarItem)
