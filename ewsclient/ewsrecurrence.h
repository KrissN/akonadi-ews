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

#ifndef EWSRECURRENCE_H
#define EWSRECURRENCE_H

#include <KCalCore/Recurrence>

class QXmlStreamReader;

class EwsRecurrence : public KCalCore::Recurrence
{
public:
    EwsRecurrence();
    EwsRecurrence(QXmlStreamReader &reader);
    EwsRecurrence(const EwsRecurrence &other);
    EwsRecurrence &operator=(const EwsRecurrence &other);
private:
    bool readRelativeYearlyRecurrence(QXmlStreamReader &reader);
    bool readAbsoluteYearlyRecurrence(QXmlStreamReader &reader);
    bool readRelativeMonthlyRecurrence(QXmlStreamReader &reader);
    bool readAbsoluteMonthlyRecurrence(QXmlStreamReader &reader);
    bool readWeeklyRecurrence(QXmlStreamReader &reader);
    bool readDailyRecurrence(QXmlStreamReader &reader);
    bool readEndDateRecurrence(QXmlStreamReader &reader);
    bool readNumberedRecurrence(QXmlStreamReader &reader);

    bool readDow(QXmlStreamReader &reader, QBitArray &dow);
};

Q_DECLARE_METATYPE(EwsRecurrence)

#endif
