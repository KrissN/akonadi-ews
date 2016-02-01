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

#include <QtCore/QXmlStreamWriter>

#include "ewsunsubscriberequest.h"
#include "ewsclient_debug.h"

EwsUnsubscribeRequest::EwsUnsubscribeRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsUnsubscribeRequest::~EwsUnsubscribeRequest()
{
}

void EwsUnsubscribeRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

   startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("Unsubscribe"));

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("SubscriptionId"), mSubscriptionId);

    writer.writeEndElement();   // Unsubscribe

    endSoapDocument(writer);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsUnsubscribeRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("Unsubscribe"),
                                [this](QXmlStreamReader &reader) {return parseUnsubscribeResponse(reader);});
}

bool EwsUnsubscribeRequest::parseUnsubscribeResponse(QXmlStreamReader &reader)
{
    QSharedPointer<Response> resp(new Response(reader));
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mResponse = resp;
    return true;
}

EwsUnsubscribeRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
}
