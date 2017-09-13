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

#include "ewsmailbox.h"

#include <QSharedData>

#include <KMime/HeaderParsing>

#include "ewsclient_debug.h"

class EwsMailboxPrivate : public QSharedData
{
public:
    EwsMailboxPrivate();
    virtual ~EwsMailboxPrivate();

    bool mValid;

    QString mName;
    QString mEmail;
};

EwsMailboxPrivate::EwsMailboxPrivate()
    : mValid(false)
{
}

EwsMailbox::EwsMailbox()
    : d(new EwsMailboxPrivate())
{
}

EwsMailbox::EwsMailbox(QXmlStreamReader &reader)
    : d(new EwsMailboxPrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in mailbox element:")
                                    << reader.namespaceUri();
            return;
        }

        if (reader.name() == QStringLiteral("Name")) {
            d->mName = reader.readElementText();
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read EWS request - invalid mailbox Name element.");
                return;
            }
        } else if (reader.name() == QStringLiteral("EmailAddress")) {
            d->mEmail = reader.readElementText();
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read EWS request - invalid mailbox EmailAddress element.");
                return;
            }
        } else if (reader.name() == QStringLiteral("RoutingType") ||
                   reader.name() == QStringLiteral("MailboxType") ||
                   reader.name() == QStringLiteral("ItemId")) {
            // Unsupported - ignore
            //qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unsupported mailbox element %1").arg(reader.name().toString());
            reader.skipCurrentElement();
        }
    }

    d->mValid = true;
}

EwsMailboxPrivate::~EwsMailboxPrivate()
{
}

EwsMailbox::EwsMailbox(const EwsMailbox &other)
    : d(other.d)
{
}

EwsMailbox::EwsMailbox(EwsMailbox &&other)
    : d(std::move(other.d))
{
}

EwsMailbox::~EwsMailbox()
{
}

EwsMailbox &EwsMailbox::operator=(const EwsMailbox &other)
{
    d = other.d;
    return *this;
}
EwsMailbox &EwsMailbox::operator=(EwsMailbox &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsMailbox::isValid() const
{
    return d->mValid;
}

QString EwsMailbox::name() const
{
    return d->mName;
}

QString EwsMailbox::email() const
{
    return d->mEmail;
}

QString EwsMailbox::emailWithName() const
{
    if (d->mName.isEmpty()) {
        return d->mEmail;
    } else {
        return QStringLiteral("%1 <%2>").arg(d->mName).arg(d->mEmail);
    }
}

EwsMailbox::operator KMime::Types::Mailbox() const
{
    KMime::Types::Mailbox mbox;
    mbox.setAddress(d->mEmail.toAscii());
    if (!d->mName.isEmpty()) {
        mbox.setName(d->mName);
    }
    return mbox;
}
