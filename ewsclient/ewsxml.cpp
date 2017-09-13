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

#include "ewsxml.h"

#include <QDateTime>

#include "ewsclient_debug.h"
#include "ewsfolder.h"
#include "ewsid.h"
#include "ewsitem.h"

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
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Error reading %1 element")
                                .arg(reader.name().toString());
        reader.skipCurrentElement();
        return false;
    }
    if (elmText == QStringLiteral("true")) {
        val = true;
    } else if (elmText == QStringLiteral("false")) {
        val = false;
    } else {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected invalid boolean value in %1 element:")
                                .arg(reader.name().toString())
                                << elmText;
        return false;
    }

    return true;
}

bool ewsXmlBoolWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(val.toBool() ? QStringLiteral("true") : QStringLiteral("false"));

    return true;
}

bool ewsXmlBase64Reader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = QByteArray::fromBase64(reader.readElementText().toLatin1());
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }

    return true;
}

bool ewsXmlBase64Writer(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(QString::fromLatin1(val.toByteArray().toBase64()));

    return true;
}

bool ewsXmlIdReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsId id = EwsId(reader);
    if (id.type() == EwsId::Unspecified) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(id);
    reader.skipCurrentElement();
    return true;
}

bool ewsXmlIdWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    EwsId id = val.value<EwsId>();
    if (id.type() == EwsId::Unspecified) {
        return false;
    }

    id.writeAttributes(writer);

    return true;
}

bool ewsXmlTextReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    val = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    return true;
}

bool ewsXmlTextWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(val.toString());

    return true;
}

bool ewsXmlUIntReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    bool ok;
    val = reader.readElementText().toUInt(&ok);
    if (reader.error() != QXmlStreamReader::NoError || !ok) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        return false;
    }
    return true;
}

bool ewsXmlUIntWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    writer.writeCharacters(QString::number(val.toUInt()));

    return true;
}

bool ewsXmlDateTimeReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    QDateTime dt = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
    if (reader.error() != QXmlStreamReader::NoError || !dt.isValid()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        return false;
    }
    val = QVariant::fromValue<QDateTime>(dt);
    return true;
}

bool ewsXmlItemReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsItem item = EwsItem(reader);
    if (!item.isValid()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(item);
    return true;
}

bool ewsXmlFolderReader(QXmlStreamReader &reader, QVariant &val)
{
    QString elmName = reader.name().toString();
    EwsFolder folder = EwsFolder(reader);
    if (!folder.isValid()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue(folder);
    return true;
}

bool ewsXmlEnumReader(QXmlStreamReader &reader, QVariant &val, QVector<QString> items)
{
    QString elmName = reader.name().toString();
    QString text = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                                .arg(elmName);
        reader.skipCurrentElement();
        return false;
    }
    int i = 0;
    QVector<QString>::const_iterator it;
    for (it = items.cbegin(); it != items.cend(); it++, i++) {
        if (text == *it) {
            val = i;
            break;
        }
    }

    if (it == items.cend()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - unknown value %2.")
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

template <>
QString readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    ok = true;
    QStringRef elmName = reader.name();
    QString val = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                .arg(parentElement).arg(elmName.toString());
        reader.skipCurrentElement();
        val.clear();
        ok = false;
    }

    return val;
}

template <>
int readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QStringRef elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    int val = 0;
    if (ok) {
        val = valStr.toInt(&ok);
        if (!ok) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                    .arg(parentElement).arg(elmName.toString());
        }
    }

    return val;
}

template <>
long readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QStringRef elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    long val = 0;
    if (ok) {
        val = valStr.toLong(&ok);
        if (!ok) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                    .arg(parentElement).arg(elmName.toString());
        }
    }

    return val;
}

template <>
QDateTime readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QStringRef elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    QDateTime val;
    if (ok) {
        val = QDateTime::fromString(valStr, Qt::ISODate);
        if (!val.isValid()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                    .arg(parentElement).arg(elmName.toString());
            ok = false;
        }
    }

    return val;
}

template <>
bool readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QStringRef elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    bool val = false;
    if (ok) {
        if (valStr == QStringLiteral("true")) {
            val = true;
        } else if (valStr == QStringLiteral("false")) {
            val = false;
        } else {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                    .arg(parentElement).arg(elmName.toString());
            ok = false;
        }
    }

    return val;
}

template <>
QByteArray readXmlElementValue(QXmlStreamReader &reader, bool &ok, const QString &parentElement)
{
    QStringRef elmName = reader.name();
    QString valStr = readXmlElementValue<QString>(reader, ok, parentElement);
    QByteArray val;
    if (ok) {
        /* QByteArray::fromBase64() does not perform any input validity checks
           and skips invalid input characters */
        val = QByteArray::fromBase64(valStr.toAscii());
    }

    return val;
}


