/*  This file is part of Akonadi EWS Resource
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

#include "ewsitembase.h"
#include "ewsitembase_p.h"

#include "ewsclient_debug.h"

EwsItemBasePrivate::EwsItemBasePrivate()
    : mValid(false)
{
    qRegisterMetaType<EwsItemBasePrivate::PropertyHash>();
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

bool EwsItemBasePrivate::extendedPropertyReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsPropertyField prop;
    QVariant value;
    PropertyHash propHash = val.value<PropertyHash>();
    QString elmName = reader.name().toString();

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid namespace.")
                            .arg(elmName);
            reader.skipCurrentElement();
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
        else if (reader.name() == QStringLiteral("Values")) {
            QStringList values;
            while (reader.readNextStartElement()) {
                if (reader.namespaceUri() != ewsTypeNsUri) {
                    qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid namespace.")
                                    .arg(elmName);
                    reader.skipCurrentElement();
                    reader.skipCurrentElement();
                    return false;
                }

                if (reader.name() == QStringLiteral("Value")) {
                    values.append(reader.readElementText());
                }
            }
            value = values;
        }
        else {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - unexpected child element %2")
                            .arg(elmName).arg(reader.qualifiedName().toString());
            reader.skipCurrentElement();
            return false;
        }
    }
    propHash.insert(prop, value);

    val = QVariant::fromValue<PropertyHash>(propHash);

    return true;
}

bool EwsItemBasePrivate::extendedPropertyWriter(QXmlStreamWriter &writer, const QVariant &val)
{
    PropertyHash propHash = val.value<PropertyHash>();
    PropertyHash::const_iterator it;
    for (it = propHash.cbegin(); it != propHash.cend();) {
        /* EwsXml will already start the ExtendedProperty element expecting a single value.
         * This is not exactly true for extended properties as there may be many. Work around this
         * by avoiding writing the first element start tag and the last element end tag. */
        if (it != propHash.cbegin()) {
            writer.writeStartElement(ewsTypeNsUri, QStringLiteral("ExtendedProperty"));
        }
        it.key().write(writer);
        it.key().writeExtendedValue(writer, it.value());
        it++;
        if (it != propHash.cend()) {
            writer.writeEndElement();
        }
    }

    return true;
}

bool EwsItemBasePrivate::operator==(const EwsItemBasePrivate &other) const
{
    if (mValid != other.mValid) {
        return false;
    }

    return mFields == other.mFields;
}

QVariant EwsItemBase::operator[](const EwsPropertyField &prop) const
{
    EwsItemBasePrivate::PropertyHash propHash =
        d->mFields[EwsItemFieldExtendedProperties].value<EwsItemBasePrivate::PropertyHash>();
    EwsItemBasePrivate::PropertyHash::iterator it = propHash.find(prop);
    if (it != propHash.end()) {
        return it.value();
    }
    else {
        return QVariant();
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

void EwsItemBase::setField(EwsItemFields f, const QVariant& value)
{
    d->mFields[f] = value;
}

void EwsItemBase::setProperty(const EwsPropertyField &prop, const QVariant &value)
{
    EwsItemBasePrivate::PropertyHash propHash =
        d->mFields[EwsItemFieldExtendedProperties].value<EwsItemBasePrivate::PropertyHash>();
    propHash[prop] = value;
    d->mFields[EwsItemFieldExtendedProperties] = QVariant::fromValue<EwsItemBasePrivate::PropertyHash>(propHash);
}
