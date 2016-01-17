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

#include "ewsxmlreader.h"

#include <QtCore/QDateTime>
#include <KCodecs/KCodecs>

#include "ewsid.h"

#include "ewsclient_debug.h"

static const QVector<QString> messageSensitivityNames = {
    QStringLiteral("Normal"),
    QStringLiteral("Personal"),
    QStringLiteral("Private"),
    QStringLiteral("Confidential")
};

static const QVector<QString> messageImportanceNames = {
    QStringLiteral("Low"),
    QStringLiteral("Normal"),
    QStringLiteral("High")
};

static const QVector<QString> calendarItemTypeNames = {
    QStringLiteral("Single"),
    QStringLiteral("Occurrence"),
    QStringLiteral("Exception"),
    QStringLiteral("RecurringMaster")
};

static const QVector<QString> legacyFreeBusyStatusNames = {
    QStringLiteral("Free"),
    QStringLiteral("Tentative"),
    QStringLiteral("Busy"),
    QStringLiteral("OOF"),
    QStringLiteral("NoData")
};

static const QVector<QString> responseTypeNames = {
    QStringLiteral("Unknown"),
    QStringLiteral("Organizer"),
    QStringLiteral("Tentative"),
    QStringLiteral("Accept"),
    QStringLiteral("Decline"),
    QStringLiteral("NoResponseReceived")
};

bool ewsXmlBoolReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmText = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading %1 element")
                        .arg(reader.name().toString());
        reader.skipCurrentElement();
        return false;
    }
    if (elmText == QStringLiteral("true")) {
        val = true;
    }
    else if (elmText == QStringLiteral("false")) {
        val = false;
    }
    else {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unexpected invalid boolean value in %1 element:")
                        .arg(reader.name().toString())
                        << elmText;
        return false;
    }

    return true;
}

bool ewsXmlBase64Reader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = KCodecs::base64Decode(reader.readElementText().toLatin1());
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }

    return true;
}

bool ewsXmlIdReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsId id = EwsId(reader);
    if (id.type() == EwsId::Unspecified) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(id);
    reader.skipCurrentElement();
    return true;
}

bool ewsXmlTextReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    return true;
}

bool ewsXmlUIntReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    bool ok;
    val = reader.readElementText().toUInt(&ok);
    if (reader.error() != QXmlStreamReader::NoError || !ok) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        return false;
    }
    return true;
}

bool ewsXmlDateTimeReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    QDateTime dt = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
    if (reader.error() != QXmlStreamReader::NoError || !dt.isValid()) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        return false;
    }
    val = QVariant::fromValue<QDateTime>(dt);
    return true;
}

bool ewsXmlEnumReader(QXmlStreamReader &reader, QVariant &val, QVector<QString> items)
{
    QString elmName = reader.name().toString();
    QString text = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    int i;
    QVector<QString>::const_iterator it;
    for (it = items.cbegin(); it != items.cend(); it++, i++) {
        if (text == *it) {
            val = i;
            break;
        }
    }

    if (it == items.cend()) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read %1 element - unknown value %2.")
                        .arg(elmName).arg(text);
        return false;
    }
    return true;
}

bool ewsXmlSensitivityReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, messageSensitivityNames);
}

bool ewsXmlImportanceReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, messageImportanceNames);
}

bool ewsXmlCalendarItemTypeReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, calendarItemTypeNames);
}

bool ewsXmlLegacyFreeBusyStatusReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, legacyFreeBusyStatusNames);
}

bool ewsXmlResponseTypeReader(QXmlStreamReader &reader, QVariant &val)
{
    return ewsXmlEnumReader(reader, val, responseTypeNames);
}
