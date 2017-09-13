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

#include "ewscreateitemrequest.h"
#include "ewsclient_debug.h"

static const QVector<QString> messageDispositionNames = {
    QStringLiteral("SaveOnly"),
    QStringLiteral("SendOnly"),
    QStringLiteral("SendAndSaveCopy")
};

static const QVector<QString> meetingDispositionNames = {
    QStringLiteral("SendToNone"),
    QStringLiteral("SendOnlyToAll"),
    QStringLiteral("SendOnlyToChanged"),
    QStringLiteral("SendToAllAndSaveCopy"),
    QStringLiteral("SendToChangedAndSaveCopy")
};

EwsCreateItemRequest::EwsCreateItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent), mMessageDisp(EwsDispSaveOnly), mMeetingDisp(EwsMeetingDispUnspecified)
{
}

EwsCreateItemRequest::~EwsCreateItemRequest()
{
}

void EwsCreateItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("CreateItem"));

    writer.writeAttribute(QStringLiteral("MessageDisposition"),
                          messageDispositionNames[mMessageDisp]);

    if (mMeetingDisp != EwsMeetingDispUnspecified) {
        writer.writeAttribute(QStringLiteral("SendMeetingInvitations"),
                              meetingDispositionNames[mMeetingDisp]);
    }

    if (mMessageDisp == EwsDispSaveOnly || mMessageDisp == EwsDispSendAndSaveCopy) {
        writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SavedItemFolderId"));
        mSavedFolderId.writeFolderIds(writer);
        writer.writeEndElement();
    }

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("Items"));
    Q_FOREACH (const EwsItem &item, mItems) {
        item.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting CreateItem request (%1 items, parent %2)")
                                  .arg(mItems.size()).arg(mSavedFolderId.id());

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsCreateItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("CreateItem"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsCreateItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSRES_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            qCDebug(EWSRES_REQUEST_LOG) << QStringLiteral("Got CreateItem response - OK");
        } else {
            qCDebug(EWSRES_REQUEST_LOG) << QStringLiteral("Got CreateItem response - %1")
                                        .arg(resp.responseMessage());
        }
    }
    mResponses.append(resp);
    return true;
}

EwsCreateItemRequest::Response::Response(QXmlStreamReader &reader)
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
        } else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}
