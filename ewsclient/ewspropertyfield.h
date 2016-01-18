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

#ifndef EWSPROPERTYFIELD_H
#define EWSPROPERTYFIELD_H

#include <QtCore/QXmlStreamWriter>
#include <QtCore/QSharedDataPointer>

#include "ewstypes.h"

class EwsPropertyFieldPrivate;

class EwsPropertyField
{
public:
    EwsPropertyField();
    EwsPropertyField(QString uri);  // FieldURI
    EwsPropertyField(QString uri, unsigned index);   // IndexedFieldURI
    EwsPropertyField(EwsDistinguishedPropSetId psid, unsigned id, EwsPropertyType type);
    EwsPropertyField(EwsDistinguishedPropSetId psid, QString name, EwsPropertyType type);
    EwsPropertyField(QString psid, unsigned id, EwsPropertyType type);
    EwsPropertyField(QString psid, QString name, EwsPropertyType type);
    EwsPropertyField(unsigned tag, EwsPropertyType type);
    EwsPropertyField(const EwsPropertyField &other);
    ~EwsPropertyField();

    EwsPropertyField& operator=(const EwsPropertyField &other);
    bool operator==(const EwsPropertyField &other) const;

    EwsPropertyField(EwsPropertyField &&other);
    EwsPropertyField& operator=(EwsPropertyField &&other);

    void write(QXmlStreamWriter &writer) const;
    bool read(QXmlStreamReader &reader);

    bool writeValue(QXmlStreamWriter &writer, const QString &value) const;
private:
    QSharedDataPointer<EwsPropertyFieldPrivate> d;

    friend uint qHash(const EwsPropertyField &prop, uint seed);
    friend QDebug operator<<(QDebug debug, const EwsPropertyField &prop);
};

uint qHash(const EwsPropertyField &prop, uint seed);

QDebug operator<<(QDebug debug, const EwsPropertyField &prop);

#endif
