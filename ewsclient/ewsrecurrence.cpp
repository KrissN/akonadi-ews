/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewsrecurrence.h"

#include <QBitArray>
#include <QDate>
#include <QXmlStreamReader>

#include "ewsclient_debug.h"
#include "ewstypes.h"

using namespace KCalCore;

static const QString dayOfWeekNames[] = {
    QStringLiteral("Monday"),
    QStringLiteral("Tuesday"),
    QStringLiteral("Wednesday"),
    QStringLiteral("Thursday"),
    QStringLiteral("Friday"),
    QStringLiteral("Saturday"),
    QStringLiteral("Sunday"),
    QStringLiteral("Day"),
    QStringLiteral("Weekday"),
    QStringLiteral("WeekendDay"),
};
Q_CONSTEXPR unsigned dayOfWeekNameCount = sizeof(dayOfWeekNames) / sizeof(dayOfWeekNames[0]);

static const QString dayOfWeekIndexNames[] = {
    QStringLiteral("First"),
    QStringLiteral("Second"),
    QStringLiteral("Third"),
    QStringLiteral("Fourth"),
    QStringLiteral("Last")
};
Q_CONSTEXPR unsigned dayOfWeekIndexNameCount = sizeof(dayOfWeekIndexNames) / sizeof(dayOfWeekIndexNames[0]);

static const QString monthNames[] = {
    QStringLiteral("January"),
    QStringLiteral("February"),
    QStringLiteral("March"),
    QStringLiteral("April"),
    QStringLiteral("May"),
    QStringLiteral("June"),
    QStringLiteral("July"),
    QStringLiteral("August"),
    QStringLiteral("September"),
    QStringLiteral("October"),
    QStringLiteral("November"),
    QStringLiteral("December")
};
Q_CONSTEXPR unsigned monthNameCount = sizeof(monthNames) / sizeof(monthNames[0]);

EwsRecurrence::EwsRecurrence()
    : Recurrence()
{
}

EwsRecurrence::EwsRecurrence(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return;
        }

        if (elmName == QStringLiteral("RelativeYearlyRecurrence")) {
            if (!readRelativeYearlyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("AbsoluteYearlyRecurrence")) {
            if (!readAbsoluteYearlyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("RelativeMonthlyRecurrence")) {
            if (!readRelativeMonthlyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("AbsoluteMonthlyRecurrence")) {
            if (!readAbsoluteMonthlyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("WeeklyRecurrence")) {
            if (!readWeeklyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("DailyRecurrence")) {
            if (!readWeeklyRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("NoEndRecurrence")) {
            // Ignore - this is the default
            reader.skipCurrentElement();
        } else if (elmName == QStringLiteral("EndDateRecurrence")) {
            if (!readEndDateRecurrence(reader)) {
                return;
            }
        } else if (elmName == QStringLiteral("NumberedRecurrence")) {
            if (!readNumberedRecurrence(reader)) {
                return;
            }
        } else {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                    .arg(QStringLiteral("Recurrence")).arg(elmName);
            return;
        }
    }
}

EwsRecurrence::EwsRecurrence(const EwsRecurrence &other)
    : Recurrence(other)
{

}

EwsRecurrence &EwsRecurrence::operator=(const EwsRecurrence &other)
{
    *static_cast<Recurrence*>(this) = static_cast<Recurrence>(other);

    return *this;
}

bool EwsRecurrence::readRelativeYearlyRecurrence(QXmlStreamReader &reader)
{
    QBitArray dow(7);
    short dowWeekIndex = 0;
    short month = 0;
    bool hasDow = false;
    bool hasDowWeekIndex = false;
    bool hasMonth = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("DaysOfWeek")) {
            if (!readDow(reader, dow)) {
                return false;
            }
            hasDow = true;
        } else if (elmName == QStringLiteral("DayOfWeekIndex")) {
            bool ok;
            QString text = reader.readElementText();
            dowWeekIndex = decodeEnumString<short>(text, dayOfWeekIndexNames, dayOfWeekIndexNameCount, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("DayOfWeekIndex").arg(text));
                return false;
            }
            if (dowWeekIndex == 4) {    // "Last"
                dowWeekIndex = -1;
            }
            hasDowWeekIndex = true;
        } else if (elmName == QStringLiteral("Month")) {
            bool ok;
            QString text = reader.readElementText();
            month = decodeEnumString<short>(text, monthNames, monthNameCount, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("Month").arg(text));
                return false;
            }
            hasMonth = true;
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                  .arg(QStringLiteral("RelativeYearlyRecurrence")).arg(elmName);
            return false;
        }
    }

    if (!hasMonth || !hasDow || !hasDowWeekIndex) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - expected all of Month, DaysOfWeek and DayOfWeekIndex elements.");
        return false;
    }

    addYearlyMonth(month);
    addYearlyPos(dowWeekIndex, dow);

    return true;
}

bool EwsRecurrence::readAbsoluteYearlyRecurrence(QXmlStreamReader &reader)
{
    short dom = 0;
    short month = 0;
    bool hasDom = false;
    bool hasMonth = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("DayOfMonth")) {
            bool ok;
            QString text = reader.readElementText();
            dom = text.toShort(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("DayOfMonth").arg(text));
                return false;
            }
            hasDom = true;
        } else if (elmName == QStringLiteral("Month")) {
            bool ok;
            QString text = reader.readElementText();
            month = decodeEnumString<short>(text, monthNames, monthNameCount, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("Month").arg(text));
                return false;
            }
            hasMonth = true;
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read recurrence element - unknown element: %1.")
                                  .arg(elmName);
            return false;
        }
    }

    if (!hasDom || !hasMonth) {
        qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read recurrence element - need both month and dom values.");
        return false;
    }

    // "If for a particular month this value is larger than the number of days in the month,
    //  the last day of the month is assumed for this property."
    QDate date(2000, month, 1);
    if (dom > date.daysInMonth()) {
        dom = -1;
    }

    addYearlyMonth(month);
    addYearlyDay(dom);

    return true;
}

bool EwsRecurrence::readRelativeMonthlyRecurrence(QXmlStreamReader &reader)
{
    QBitArray dow(7);
    short dowWeekIndex = 0;
    int interval = 0;
    bool hasDow = false;
    bool hasDowWeekIndex = false;
    bool hasInterval = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("Interval")) {
            bool ok;
            QString text = reader.readElementText();
            interval = text.toInt(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("Interval").arg(text));
                return false;
            }
            hasInterval = true;
        } else if (elmName == QStringLiteral("DaysOfWeek")) {
            if (!readDow(reader, dow)) {
                return false;
            }
            hasDow = true;
        } else if (elmName == QStringLiteral("DayOfWeekIndex")) {
            bool ok;
            QString text = reader.readElementText();
            dowWeekIndex = decodeEnumString<short>(text, dayOfWeekIndexNames, dayOfWeekIndexNameCount, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("DayOfWeekIndex").arg(text));
                return false;
            }
            if (dowWeekIndex == 4) {    // "Last"
                dowWeekIndex = -1;
            }
            hasDowWeekIndex = true;
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - unknown element: %1.")
                                  .arg(elmName);
            return false;
        }
    }

    if (!hasInterval || !hasDow || !hasDowWeekIndex) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - expected all of Interval, DaysOfWeek and DayOfWeekIndex elements.");
        return false;
    }

    addMonthlyPos(dowWeekIndex, dow);
    setFrequency(interval);

    return true;
}

bool EwsRecurrence::readAbsoluteMonthlyRecurrence(QXmlStreamReader &reader)
{
    short dom = 0;
    int interval = 0;
    bool hasInterval = false;
    bool hasDom = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("Interval")) {
            bool ok;
            QString text = reader.readElementText();
            interval = text.toInt(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                        .arg(QStringLiteral("Interval").arg(text));
                return false;
            }
            hasInterval = true;
        } else if (elmName == QStringLiteral("DayOfMonth")) {
            bool ok;
            QString text = reader.readElementText();
            dom = text.toShort(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                        .arg(QStringLiteral("DayOfMonth").arg(text));
                return false;
            }
            hasDom = true;
        } else {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - unknown element: %1.")
                                    .arg(elmName);
            return false;
        }
    }

    if (!hasInterval || !hasDom) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - expected both Interval and DayOfMonth.");
        return false;
    }

    addMonthlyDate(dom);
    setFrequency(interval);

    return true;
}

bool EwsRecurrence::readWeeklyRecurrence(QXmlStreamReader &reader)
{
    int interval = 1;
    QBitArray dow(7);
    int weekStart = 0;
    bool hasInterval = false;
    bool hasDow = false;
    bool hasWeekStart = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("Interval")) {
            bool ok;
            QString text = reader.readElementText();
            interval = text.toInt(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                        .arg(QStringLiteral("Interval").arg(text));
                return false;
            }
            hasInterval = true;
        } else if (elmName == QStringLiteral("DaysOfWeek")) {
            if (!readDow(reader, dow)) {
                return false;
            }
            hasDow = true;
        } else if (elmName == QStringLiteral("FirstDayOfWeek")) {
            bool ok;
            QString text = reader.readElementText();
            weekStart = decodeEnumString<int>(text, dayOfWeekNames, dayOfWeekNameCount, &ok) + 1;
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                        .arg(QStringLiteral("FirstDayOfWeek").arg(text));
                return false;
            }
            hasWeekStart = true;
        } else {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                    .arg(QStringLiteral("WeeklyRecurrence")).arg(elmName);
            return false;
        }
    }

    if (!hasInterval || !hasDow || !hasWeekStart) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - expected all of Interval, DaysOfWeek and FirstDatOfWeek elements.");
        return false;
    }

    setWeekly(interval, dow, weekStart);

    return true;
}

bool EwsRecurrence::readDailyRecurrence(QXmlStreamReader &reader)
{
    int interval = 1;
    bool hasInterval = false;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("Interval")) {
            bool ok;
            QString text = reader.readElementText();
            interval = text.toInt(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("Interval").arg(text));
                return false;
            }
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read recurrence element - unknown element: %1.")
                                  .arg(elmName);
            return false;
        }
    }

    if (!hasInterval) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read Recurrence element - expected an Interval element.");
        return false;
    }

    setDaily(interval);

    return true;
}

bool EwsRecurrence::readEndDateRecurrence(QXmlStreamReader &reader)
{
    QDate dateEnd;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("EndDate")) {
            QString text = reader.readElementText();
            dateEnd = QDate::fromString(text, Qt::ISODate);
            if (reader.error() != QXmlStreamReader::NoError || !dateEnd.isValid()) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("EndDate").arg(text));
                return false;
            }
        } else if (elmName == QStringLiteral("StartDate")) {
            // Don't care
            reader.skipCurrentElement();
        } else {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                    .arg(QStringLiteral("EndDateRecurrence")).arg(elmName);
            return false;
        }
    }

    setEndDate(dateEnd);

    return true;
}

bool EwsRecurrence::readNumberedRecurrence(QXmlStreamReader &reader)
{
    int numOccurrences = 0;

    while (reader.readNextStartElement()) {
        QString elmName = reader.name().toString();
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(elmName)
                                    << reader.namespaceUri();
            return false;
        }

        if (elmName == QStringLiteral("NumberOfOccurrences")) {
            bool ok;
            QString text = reader.readElementText();
            numOccurrences = text.toInt(&ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                      .arg(QStringLiteral("NumberOfOccurrences").arg(text));
                return false;
            }
        } else if (elmName == QStringLiteral("StartDate")) {
            // Don't care
            reader.skipCurrentElement();
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read recurrence element - unknown element: %1.")
                                  .arg(elmName);
            return false;
        }
    }

    setDuration(numOccurrences);

    return true;
}

bool EwsRecurrence::readDow(QXmlStreamReader &reader, QBitArray &dow)
{
    bool ok;
    QString text = reader.readElementText();
    QStringList days = text.split(QChar::fromLatin1(' '));
    Q_FOREACH (const QString &day, days) {
        short dowIndex = decodeEnumString<short>(day, dayOfWeekNames, dayOfWeekNameCount, &ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element (value: %2).")
                                  .arg(QStringLiteral("DaysOfWeek").arg(day));
            return false;
        }
        if (dowIndex == 7) { // "Day"
            dow.fill(true, 0, 7);
        } else if (dowIndex == 8) { // "Weekday"
            dow.fill(true, 0, 5);
        } else if (dowIndex == 8) { // "WeekendDay"
            dow.fill(true, 5, 7);
        } else {
            dow.setBit(dowIndex);
        }
    }
    return true;
}

