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

#ifndef EWSOCCURRENCE_H
#define EWSOCCURRENCE_H

#include <QDateTime>
#include <QMetaType>
#include <QSharedDataPointer>

#include "ewstypes.h"

class EwsOccurrencePrivate;
class EwsId;
class QXmlStreamReader;

class EwsOccurrence
{
public:
    typedef QList<EwsOccurrence> List;

    EwsOccurrence();
    explicit EwsOccurrence(QXmlStreamReader &reader);
    EwsOccurrence(const EwsOccurrence &other);
    EwsOccurrence(EwsOccurrence &&other);
    virtual ~EwsOccurrence();

    EwsOccurrence& operator=(const EwsOccurrence &other);
    EwsOccurrence& operator=(EwsOccurrence &&other);

    bool isValid() const;
    const EwsId &itemId() const;
    QDateTime start() const;
    QDateTime end() const;
    QDateTime originalStart() const;
protected:
    QSharedDataPointer<EwsOccurrencePrivate> d;
};

Q_DECLARE_METATYPE(EwsOccurrence)
Q_DECLARE_METATYPE(EwsOccurrence::List)

#endif
