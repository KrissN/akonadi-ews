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

#include "ewsattendee.h"

#include <QSharedData>
#include <QXmlStreamReader>

#include "ewsclient_debug.h"
#include "ewsmailbox.h"
#include "ewstypes.h"

class EwsAttendeePrivate : public QSharedData
{
public:
    EwsAttendeePrivate();
    virtual ~EwsAttendeePrivate();

    bool mValid;

    EwsMailbox mMailbox;
    EwsEventResponseType mResponse;
    QDateTime mResponseTime;
};

static const QString responseTypeNames[] = {
    QStringLiteral("Unknown"),
    QStringLiteral("Organizer"),
    QStringLiteral("Tentative"),
    QStringLiteral("Accept"),
    QStringLiteral("Decline"),
    QStringLiteral("NoResponseReceived")
};
Q_CONSTEXPR unsigned responseTypeNameCount = sizeof(responseTypeNames) / sizeof(responseTypeNames[0]);

EwsAttendeePrivate::EwsAttendeePrivate()
    : mValid(false), mResponse(EwsEventResponseNotReceived)
{
}

EwsAttendee::EwsAttendee()
    : d(new EwsAttendeePrivate())
{
}

EwsAttendee::EwsAttendee(QXmlStreamReader &reader)
    : d(new EwsAttendeePrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in mailbox element:")
                                    << reader.namespaceUri();
            return;
        }

        if (reader.name() == QStringLiteral("Mailbox")) {
            d->mMailbox = EwsMailbox(reader);
            if (!d->mMailbox.isValid()) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid attendee %1 element.")
                                      .arg(reader.name().toString());
                return;
            }
        } else if (reader.name() == QStringLiteral("ResponseType")) {
            bool ok;
            d->mResponse = decodeEnumString<EwsEventResponseType>(reader.readElementText(),
                           responseTypeNames, responseTypeNameCount, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid attendee %1 element.")
                                      .arg(reader.name().toString());
                return;
            }
        } else if (reader.name() == QStringLiteral("LastResponseTime")) {
            // Unsupported - ignore
            //qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unsupported mailbox element %1").arg(reader.name().toString());
            reader.skipCurrentElement();
        }
    }

    d->mValid = true;
}

EwsAttendeePrivate::~EwsAttendeePrivate()
{
}

EwsAttendee::EwsAttendee(const EwsAttendee &other)
    : d(other.d)
{
}

EwsAttendee::EwsAttendee(EwsAttendee &&other)
    : d(std::move(other.d))
{
}

EwsAttendee::~EwsAttendee()
{
}

EwsAttendee &EwsAttendee::operator=(const EwsAttendee &other)
{
    d = other.d;
    return *this;
}
EwsAttendee &EwsAttendee::operator=(EwsAttendee &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsAttendee::isValid() const
{
    return d->mValid;
}

const EwsMailbox &EwsAttendee::mailbox() const
{
    return d->mMailbox;
}

EwsEventResponseType EwsAttendee::response() const
{
    return d->mResponse;
}

QDateTime EwsAttendee::responseTime() const
{
    return d->mResponseTime;
}
