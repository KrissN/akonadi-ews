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

#ifndef EWSGETEVENTSREQUEST_H
#define EWSGETEVENTSREQUEST_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QDateTime>

#include "ewsid.h"
#include "ewseventrequestbase.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsGetEventsRequest : public EwsEventRequestBase
{
    Q_OBJECT
public:
    EwsGetEventsRequest(EwsClient &client, QObject *parent);
    virtual ~EwsGetEventsRequest();

    void setWatermark(const QString &watermark) { mWatermark = watermark; };

    virtual void start() Q_DECL_OVERRIDE;

protected:
    QString mWatermark;
};

#endif
