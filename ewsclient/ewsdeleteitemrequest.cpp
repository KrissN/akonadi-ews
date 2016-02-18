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

#include "ewsdeleteitemrequest.h"
#include "ewsclient_debug.h"

static const QVector<QString> deleteTypes = {
    QStringLiteral("HardDelete"),
    QStringLiteral("SoftDelete"),
    QStringLiteral("MoveToDeletedItems")
};

EwsDeleteItemRequest::EwsDeleteItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent), mType(SoftDelete)
{
}

EwsDeleteItemRequest::~EwsDeleteItemRequest()
{
}

void EwsDeleteItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("DeleteItem"));

    writer.writeAttribute(QStringLiteral("DeleteType"), deleteTypes[mType]);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemIds"));
    Q_FOREACH(const EwsId &id, mIds) {
        id.writeItemIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsDeleteItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("DeleteItem"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsDeleteItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    mResponses.append(resp);
    return true;
}

EwsDeleteItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(QStringLiteral("Unexpected namespace in %1 element: %2")
                .arg(QStringLiteral("ResponseMessage")).arg(reader.namespaceUri().toString()));
            return;
        }

        if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}
