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

#include "ewseffectiverights.h"

#include <QBitArray>
#include <QSharedData>
#include <QXmlStreamReader>

#include "ewsclient_debug.h"
#include "ewstypes.h"

class EwsEffectiveRightsPrivate : public QSharedData
{
public:
    enum Right {
        CreateAssociated = 0,
        CreateContents,
        CreateHierarchy,
        Delete,
        Modify,
        Read,
        ViewPrivateItems
    };

    EwsEffectiveRightsPrivate();
    virtual ~EwsEffectiveRightsPrivate();

    bool readRight(QXmlStreamReader &reader, Right right);

    bool mValid;

    QBitArray mRights;
};

EwsEffectiveRightsPrivate::EwsEffectiveRightsPrivate()
    : mValid(false), mRights(7)
{
}

EwsEffectiveRightsPrivate::~EwsEffectiveRightsPrivate()
{
}

bool EwsEffectiveRightsPrivate::readRight(QXmlStreamReader &reader, Right right)
{
    QString elm = reader.name().toString();
    QString text = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                              .arg(QStringLiteral("EffectiveRights")).arg(elm);
        return false;
    }

    if (text == QStringLiteral("true")) {
        mRights.setBit(right);
    } else if (text == QStringLiteral("false")) {
        mRights.clearBit(right);
    } else {
        qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element value: %3.")
                              .arg(QStringLiteral("EffectiveRights")).arg(elm).arg(text);
        return false;
    }

    return true;
}

EwsEffectiveRights::EwsEffectiveRights()
    : d(new EwsEffectiveRightsPrivate())
{
}

EwsEffectiveRights::EwsEffectiveRights(QXmlStreamReader &reader)
    : d(new EwsEffectiveRightsPrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in mailbox element:")
                                    << reader.namespaceUri();
            return;
        }

        if (reader.name() == QStringLiteral("CreateAssociated")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::CreateAssociated)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("CreateContents")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::CreateContents)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("CreateHierarchy")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::CreateHierarchy)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("Delete")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::Delete)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("Modify")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::Modify)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("Read")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::Read)) {
                return;
            }
        } else if (reader.name() == QStringLiteral("ViewPrivateItems")) {
            if (!d->readRight(reader, EwsEffectiveRightsPrivate::ViewPrivateItems)) {
                return;
            }
        } else {
            qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element - unknown element: %2.")
                                  .arg(QStringLiteral("EffectiveRights")).arg(reader.name().toString());
            return;
        }
    }

    d->mValid = true;
}

EwsEffectiveRights::EwsEffectiveRights(const EwsEffectiveRights &other)
    : d(other.d)
{
}

EwsEffectiveRights::EwsEffectiveRights(EwsEffectiveRights &&other)
    : d(std::move(other.d))
{
}

EwsEffectiveRights::~EwsEffectiveRights()
{
}

EwsEffectiveRights &EwsEffectiveRights::operator=(const EwsEffectiveRights &other)
{
    d = other.d;
    return *this;
}
EwsEffectiveRights &EwsEffectiveRights::operator=(EwsEffectiveRights &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsEffectiveRights::isValid() const
{
    return d->mValid;
}

bool EwsEffectiveRights::canCreateAssociated() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::CreateAssociated);
}

bool EwsEffectiveRights::canCreateContents() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::CreateContents);
}

bool EwsEffectiveRights::canCreateHierarchy() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::CreateHierarchy);
}

bool EwsEffectiveRights::canDelete() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::Delete);
}

bool EwsEffectiveRights::canModify() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::Modify);
}

bool EwsEffectiveRights::canRead() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::Read);
}

bool EwsEffectiveRights::canViewPrivateItems() const
{
    return d->mRights.testBit(EwsEffectiveRightsPrivate::ViewPrivateItems);
}

