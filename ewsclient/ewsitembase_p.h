/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

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

#include <QtCore/QSharedData>

#include "ewsid.h"

class EwsItemBasePrivate : public QSharedData
{
public:
    EwsItemBasePrivate();
    virtual ~EwsItemBasePrivate();

    template <typename T> static T decodeEnumString(QString str, const QString* table, unsigned count, bool *ok)
    {
        unsigned i;
        T enumVal;
        for (i = 0; i < count; i++) {
            if (str == table[i]) {
                enumVal = static_cast<T>(i);
                break;
            }
        }
        *ok = (i < count);
        return enumVal;
    }

    // When the item, is first constructed it will only contain the id and will therefore be
    // invalid. Once updated through EWS the remaining data will be populated and the item will
    // be valid.
    bool mValid;

    QHash<EwsItemFields, QVariant> mFields;

    QHash<EwsPropertyField, QString> mProperties;
};

#endif
