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

#include "ewsitembase.h"
#include "ewsitembase_p.h"

#include "ewsclient_debug.h"

EwsItemBasePrivate::EwsItemBasePrivate()
    : mValid(false)
{
}

EwsItemBasePrivate::~EwsItemBasePrivate()
{
}

EwsItemBase::EwsItemBase(QSharedDataPointer<EwsItemBasePrivate> priv)
    : d(priv)
{
}

EwsItemBase::EwsItemBase(const EwsItemBase &other)
    : d(other.d)
{
}

EwsItemBase::EwsItemBase(EwsItemBase &&other)
    : d(std::move(other.d))
{
}

EwsItemBase::~EwsItemBase()
{
}

bool EwsItemBase::isValid() const
{
    return d->mValid;
}

bool EwsItemBase::readExtendedProperty(QXmlStreamReader &reader)
{
    EwsPropertyField prop;
    QString value;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid namespace");
            return false;
        }

        if (reader.name() == QStringLiteral("FieldURI")
            || reader.name() == QStringLiteral("IndexedFieldURI")
            || reader.name() == QStringLiteral("ExtendedFieldURI")) {
            if (!prop.read(reader)) {
                return false;
                reader.skipCurrentElement();
            }
            reader.skipCurrentElement();
        }
        else if (reader.name() == QStringLiteral("Value")) {
            value = reader.readElementText();
        }
        else {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS 1request - unexpected element %1")
                            .arg(reader.qualifiedName().toString());
            reader.skipCurrentElement();
            return false;
        }
    }
    d->mProperties.insert(prop, value);

    return true;
}

QStringRef EwsItemBase::operator[](const EwsPropertyField &prop) const
{
    QHash<EwsPropertyField, QString>::const_iterator it = d->mProperties.find(prop);
    if (it != d->mProperties.cend()) {
        return QStringRef(&it.value());
    }
    else {
        return QStringRef();
    }
}

bool EwsItemBase::hasField(EwsItemFields f) const
{
    return d->mFields.contains(f);
}

QVariant EwsItemBase::operator[](EwsItemFields f) const
{
    if (hasField(f)) {
        return d->mFields[f];
    }
    else {
        return QVariant();
    }
}


