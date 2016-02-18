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

#include "ewsgeteventsrequest.h"
#include "ewsxml.h"
#include "ewsclient_debug.h"

EwsGetEventsRequest::EwsGetEventsRequest(EwsClient &client, QObject *parent)
    : EwsEventRequestBase(client, QStringLiteral("GetEvents"), parent)
{
}

EwsGetEventsRequest::~EwsGetEventsRequest()
{
}

void EwsGetEventsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetEvents"));

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("SubscriptionId"), mSubscriptionId);

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("Watermark"), mWatermark);

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting GetEvents request (subId: %1, wmark: %2)")
                    .arg(mSubscriptionId).arg(mWatermark);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}
