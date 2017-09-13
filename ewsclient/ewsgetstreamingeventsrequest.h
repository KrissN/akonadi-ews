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

#ifndef EWSGETSTREAMINGEVENTSREQUEST_H
#define EWSGETSTREAMINGEVENTSREQUEST_H

#include <QList>
#include <QSharedPointer>
#include <QTimer>

#include "ewseventrequestbase.h"
#include "ewsid.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsGetStreamingEventsRequest : public EwsEventRequestBase
{
    Q_OBJECT
public:
    EwsGetStreamingEventsRequest(EwsClient &client, QObject *parent);
    virtual ~EwsGetStreamingEventsRequest();

    void setTimeout(uint timeout)
    {
        mTimeout = timeout;
    };

    virtual void start() override;
public Q_SLOTS:
    void eventsProcessed(const Response &response);
Q_SIGNALS:
    void eventsReceived(KJob *job);
protected Q_SLOTS:
    void requestData(KIO::Job *job, const QByteArray &data);
    void requestDataTimeout();
protected:
    uint mTimeout;
    QTimer mRespTimer;
};

#endif
