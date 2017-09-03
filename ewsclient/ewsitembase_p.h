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

#ifndef EWSITEMBASE_P_H
#define EWSITEMBASE_P_H

#include <QSharedData>

#include "ewsid.h"

class EwsItemBasePrivate : public QSharedData
{
public:
    typedef QHash<EwsPropertyField, QVariant> PropertyHash;

    EwsItemBasePrivate();
    virtual ~EwsItemBasePrivate();

    virtual EwsItemBasePrivate *clone() const = 0;

    static bool extendedPropertyReader(QXmlStreamReader &reader, QVariant &val);
    static bool extendedPropertyWriter(QXmlStreamWriter &writer, const QVariant &val);

    // When the item, is first constructed it will only contain the id and will therefore be
    // invalid. Once updated through EWS the remaining data will be populated and the item will
    // be valid.
    bool mValid;

    QHash<EwsItemFields, QVariant> mFields;

    bool operator==(const EwsItemBasePrivate &other) const;
};

template <>
Q_INLINE_TEMPLATE EwsItemBasePrivate *QSharedDataPointer<EwsItemBasePrivate>::clone()
{
    return d->clone();
}

Q_DECLARE_METATYPE(EwsItemBasePrivate::PropertyHash)

#endif
