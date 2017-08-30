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

#include "ewsmoveitemrequest.h"
#include "ewsclient_debug.h"

EwsMoveItemRequest::EwsMoveItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsMoveItemRequest::~EwsMoveItemRequest()
{
}

void EwsMoveItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("MoveItem"));

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ToFolderId"));
    mDestFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemIds"));
    Q_FOREACH(const EwsId &id, mIds) {
        id.writeItemIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNCS(EWSRES_REQUEST_LOG) << QStringLiteral("Starting MoveItem request (") << mIds << "to" << mDestFolderId << ")";

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsMoveItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("MoveItem"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsMoveItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got MoveItem response - OK");
        }
        else {
            qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Got MoveItem response - %1")
                .arg(resp.responseMessage());
        }
    }
    mResponses.append(resp);
    return true;
}

EwsMoveItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
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

        if (reader.name() == QStringLiteral("Items")) {
            if (reader.readNextStartElement()) {
                EwsItem item(reader);
                if (!item.isValid()) {
                    return;
                }
                mId = item[EwsItemFieldItemId].value<EwsId>();

                // Finish the Items element.
                reader.skipCurrentElement();
            }
        }
        else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}
