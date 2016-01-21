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

#include "ewsgetitemrequest.h"
#include "ewsclient_debug.h"

EwsGetItemRequest::EwsGetItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsGetItemRequest::~EwsGetItemRequest()
{
}

void EwsGetItemRequest::setItemIds(const EwsId::List &ids)
{
    mIds = ids;
}

void EwsGetItemRequest::setItemShape(const EwsItemShape &shape)
{
    mShape = shape;
}

void EwsGetItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetItem"));

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemIds"));
    Q_FOREACH(const EwsId &id, mIds) {
        id.writeItemIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLIENT_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsGetItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("GetItem"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsGetItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    mResponses.append(resp);

    return true;
}

EwsGetItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(QStringLiteral("Unexpected namespace in %1 element: %2")
                .arg(QStringLiteral("ResponseMessage")).arg(reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == QStringLiteral("Items")) {
            if (!parseItems(reader)) {
                return;
            }
        }
        else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
    if (mClass != EwsResponseSuccess) {
        reader.skipCurrentElement();
    }
}

bool EwsGetItemRequest::Response::parseItems(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri || reader.name() != QStringLiteral("Items")) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected Items element (got %1).")
                        .arg(reader.qualifiedName().toString()));
    }

    if (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri)
            return setErrorMsg(QStringLiteral("Failed to read EWS request - expected child element from types namespace."));

        EwsItem item(reader);
        if (!item.isValid()) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - invalid Item element."));
        }
        mItem = item;

        // Finish the Items element.
        reader.skipCurrentElement();
    }
    return true;
}
