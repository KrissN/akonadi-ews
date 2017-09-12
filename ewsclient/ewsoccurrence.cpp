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

#include "ewsoccurrence.h"

#include <QSharedData>
#include <QXmlStreamReader>

#include "ewsclient_debug.h"
#include "ewsid.h"
#include "ewstypes.h"

class EwsOccurrencePrivate : public QSharedData
{
public:
    EwsOccurrencePrivate();
    virtual ~EwsOccurrencePrivate();

    bool mValid;

    EwsId mItemId;
    QDateTime mStart;
    QDateTime mEnd;
    QDateTime mOriginalStart;
};

EwsOccurrencePrivate::EwsOccurrencePrivate()
    : mValid(false)
{
}

EwsOccurrence::EwsOccurrence()
    : d(new EwsOccurrencePrivate())
{
}

EwsOccurrence::EwsOccurrence(QXmlStreamReader &reader)
    : d(new EwsOccurrencePrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in mailbox element:")
                            << reader.namespaceUri();
            return;
        }

        if (reader.name() == QStringLiteral("ItemId")) {
            d->mItemId = EwsId(reader);
            reader.skipCurrentElement();
        }
        else if (reader.name() == QStringLiteral("Start")) {
            d->mStart = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
            if (reader.error() != QXmlStreamReader::NoError || !d->mStart.isValid()) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                .arg(QStringLiteral("Occurrence")).arg(QStringLiteral("Start"));
                return;
            }
        }
        else if (reader.name() == QStringLiteral("End")) {
            d->mEnd = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
            if (reader.error() != QXmlStreamReader::NoError || !d->mStart.isValid()) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                .arg(QStringLiteral("Occurrence")).arg(QStringLiteral("End"));
                return;
            }
        }
        else if (reader.name() == QStringLiteral("OriginalStart")) {
            d->mStart = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
            if (reader.error() != QXmlStreamReader::NoError || !d->mStart.isValid()) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                    .arg(QStringLiteral("Occurrence")).arg(QStringLiteral("OriginalStart"));
                return;
            }
        }
        else {
            qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                .arg(QStringLiteral("Occurrence")).arg(reader.name().toString());
            return;
        }
    }

    d->mValid = true;
}

EwsOccurrencePrivate::~EwsOccurrencePrivate()
{
}

EwsOccurrence::EwsOccurrence(const EwsOccurrence &other)
    : d(other.d)
{
}

EwsOccurrence::EwsOccurrence(EwsOccurrence &&other)
    : d(std::move(other.d))
{
}

EwsOccurrence::~EwsOccurrence()
{
}

EwsOccurrence &EwsOccurrence::operator=(const EwsOccurrence &other)
{
    d = other.d;
    return *this;
}
EwsOccurrence &EwsOccurrence::operator=(EwsOccurrence &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsOccurrence::isValid() const
{
    return d->mValid;
}

const EwsId &EwsOccurrence::itemId() const
{
    return d->mItemId;
}

QDateTime EwsOccurrence::start() const
{
    return d->mStart;
}

QDateTime EwsOccurrence::end() const
{
    return d->mEnd;
}

QDateTime EwsOccurrence::originalStart() const
{
    return d->mOriginalStart;
}
