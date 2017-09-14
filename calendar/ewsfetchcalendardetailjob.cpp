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

        qDebug() << item.remoteId();

        if (!resp.isSuccess()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch item %1").arg(item.remoteId());
            continue;
        }

        const EwsItem &ewsItem = resp.item();
        QString mimeContent = ewsItem[EwsItemFieldMimeContent].toString();
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar(QStringLiteral("GMT")));
        format.fromString(memcal, mimeContent);
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Found %1 events").arg(memcal->events().count());
        KCalCore::Incidence::Ptr incidence;
        if (memcal->events().count() > 1) {
            Q_FOREACH (KCalCore::Event::Ptr event, memcal->events()) {
                qCDebugNC(EWSRES_LOG) << QString::number(event->recurrence()->recurrenceType(), 16) << event->recurrenceId().dateTime() << event->recurrenceId().isValid();
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
        KCalCore::Calendar::Ptr memcal(new KCalCore::MemoryCalendar(QStringLiteral("GMT")));
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

/* This function is a lousy workaround for Exchange returning Windows timezone names instead of
 * IANA ones.
 */
void EwsFetchCalendarDetailJob::convertTimezone(KDateTime &currentTime, const QString &msTimezone, const QString &culture)
{
    KDateTime resultDt;
    QString msTz = msTimezone;

    if (!QTimeZone::isTimeZoneIdAvailable(currentTime.timeZone().name().toLatin1())) {
        // The event uses an incorrect IANA timezone, so this will most likely be a
        // Windows timezone name. Attempt to do the conversion.

        QString ianaTz;
        // Special case:
        if (msTz == QStringLiteral("tzone://Microsoft/Utc")) {
            ianaTz = QStringLiteral("UTC");
        } else {
            QLocale locale(culture);
            if (msTz.isEmpty()) {
                msTz = currentTime.timeZone().name();
            }
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Time zone '%1' not found on IANA list. Trying to convert with country '%2'")
                                  .arg(currentTime.timeZone().name()).arg(QLocale::countryToString(locale.country()));
            qCDebugNC(EWSRES_LOG) << "MSTZ: " << msTz;
            ianaTz = QString::fromLatin1(QTimeZone::windowsIdToDefaultIanaId(msTz.toLatin1(), locale.country()));
            if (ianaTz.isEmpty()) {
                // Give it one more try with the default country.
                ianaTz = QString::fromLatin1(QTimeZone::windowsIdToDefaultIanaId(msTz.toLatin1()));
                if (ianaTz.isEmpty()) {
                    // Give it two more tries with the timezone name from the event.
                    ianaTz = QString::fromLatin1(QTimeZone::windowsIdToDefaultIanaId(currentTime.timeZone().name().toLatin1(),
                            locale.country()));
                    if (ianaTz.isEmpty()) {
                        // Give it one more try with the default country.
                        ianaTz = QString::fromLatin1(QTimeZone::windowsIdToDefaultIanaId(
                                currentTime.timeZone().name().toLatin1()));
                        qCWarningNC(EWSRES_LOG)
                                << QStringLiteral("Failed to convert time zone '%1' or '%2' to IANA id")
                                .arg(msTz).arg(currentTime.timeZone().name());
                    }
                }
            }
        }
        if (!ianaTz.isEmpty()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found IANA time zone '%1'").arg(ianaTz);
            KTimeZone newTz = KSystemTimeZones::timeZones()->zone(ianaTz);
            resultDt = KDateTime(currentTime.dateTime(), newTz);
            qCDebugNC(EWSRES_LOG) << QStringLiteral("New timezone: '%1'").arg(resultDt.timeZone().name());
        }
    }

    currentTime = resultDt;
}
