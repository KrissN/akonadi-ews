/*
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

#include "ewsdeletefolderrequest.h"
#include "ewsclient_debug.h"

static const QVector<QString> deleteTypes = {
    QStringLiteral("HardDelete"),
    QStringLiteral("SoftDelete"),
    QStringLiteral("MoveToDeletedItems")
};

EwsDeleteFolderRequest::EwsDeleteFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent), mType(SoftDelete)
{
}

EwsDeleteFolderRequest::~EwsDeleteFolderRequest()
{
}

void EwsDeleteFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("DeleteFolder"));

    writer.writeAttribute(QStringLiteral("DeleteType"), deleteTypes[mType]);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderIds"));
    Q_FOREACH (const EwsId &id, mIds) {
        id.writeFolderIds(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting DeleteFolder request (%1 folders)")
                                  .arg(mIds.size());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsDeleteFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("DeleteFolder"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsDeleteFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteFolder response - OK");
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << QStringLiteral("Got DeleteFolder response - %1")
                                        .arg(resp.responseMessage());
        }
    }

    mResponses.append(resp);
    return true;
}

EwsDeleteFolderRequest::Response::Response(QXmlStreamReader &reader)
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
