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

#include "ewscreatefolderrequest.h"
#include "ewsclient_debug.h"

EwsCreateFolderRequest::EwsCreateFolderRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
{
}

EwsCreateFolderRequest::~EwsCreateFolderRequest()
{
}

void EwsCreateFolderRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("CreateFolder"));

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ParentFolderId"));
    mParentFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("Folders"));
    Q_FOREACH(const EwsFolder &folder, mFolders) {
        folder.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting CreateFolder request (%1 folders, parent %2)")
                    .arg(mFolders.size()).arg(mParentFolderId.id());

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsCreateFolderRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("CreateFolder"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsCreateFolderRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSRES_REQUEST_LOG) << QStringLiteral("Got CreateFolder response - OK");
        }
        else {
            qCDebug(EWSRES_REQUEST_LOG) << QStringLiteral("Got CreateFolder response - %1")
                            .arg(resp.responseMessage());
        }
    }
    mResponses.append(resp);
    return true;
}

EwsCreateFolderRequest::Response::Response(QXmlStreamReader &reader)
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

        if (reader.name() == QStringLiteral("Folders")) {
            if (reader.readNextStartElement()) {
                EwsFolder folder(reader);
                if (!folder.isValid()) {
                    return;
                }
                mId = folder[EwsFolderFieldFolderId].value<EwsId>();

                // Finish the Folders element.
                reader.skipCurrentElement();
            }
        }
        else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}
