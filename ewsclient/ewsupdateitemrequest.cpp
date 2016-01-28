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

#include "ewsupdateitemrequest.h"
#include "ewsclient_debug.h"

static const QVector<QString> conflictResolutionNames = {
    QStringLiteral("NeverOverwrite"),
    QStringLiteral("AutoResolve"),
    QStringLiteral("AlwaysOverwrite")
};

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

static const QVector<QString> updateTypeElementNames = {
    QStringLiteral("AppendToItemField"),
    QStringLiteral("SetItemField"),
    QStringLiteral("DeleteItemField")
};

static const QVector<QString> itemTypeNames = {
    QStringLiteral("Item"),
    QStringLiteral("Message"),
    QStringLiteral("CalendarItem"),
    QStringLiteral("Contact"),
    QStringLiteral("DistributionList"),
    QStringLiteral("MeetingMessage"),
    QStringLiteral("MeetingRequest"),
    QStringLiteral("MeetingResponse"),
    QStringLiteral("MeetingCancellation"),
    QStringLiteral("Task")
};

EwsUpdateItemRequest::EwsUpdateItemRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent), mMessageDisp(EwsDispSaveOnly),
      mConflictResol(EwsResolAlwaysOverwrite), mMeetingDisp(EwsMeetingDispUnspecified)
{
}

EwsUpdateItemRequest::~EwsUpdateItemRequest()
{
}

void EwsUpdateItemRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("UpdateItem"));

    writer.writeAttribute(QStringLiteral("ConflictResolution"),
        conflictResolutionNames[mConflictResol]);

    writer.writeAttribute(QStringLiteral("MessageDisposition"),
        messageDispositionNames[mMessageDisp]);

    if (mMeetingDisp != EwsMeetingDispUnspecified) {
        writer.writeAttribute(QStringLiteral("SendMeetingInvitationsOrCancellations"),
            meetingDispositionNames[mMeetingDisp]);
    }

    if (mSavedFolderId.type() != EwsId::Unspecified) {
        writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SavedItemFolderId"));
        mSavedFolderId.writeFolderIds(writer);
        writer.writeEndElement();
    }

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemChanges"));
    Q_FOREACH(const ItemChange &ch, mChanges) {
        qDebug() << "writing change";
        ch.write(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsUpdateItemRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("UpdateItem"),
                                [this](QXmlStreamReader &reader) {return parseItemsResponse(reader);});
}

bool EwsUpdateItemRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    mResponses.append(resp);
    return true;
}

EwsUpdateItemRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader), mConflictCount(0)
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
            }

            // Finish the Items element.
            reader.skipCurrentElement();
        }
        else if (reader.name() == QStringLiteral("ConflictResults")) {
            if (!reader.readNextStartElement()) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - expected a %1 element inside %2 element.")
                    .arg(QStringLiteral("Value")).arg(QStringLiteral("ConflictResults")));
                return;
            }

            if (reader.name() != QStringLiteral("Count")) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - expected a %1 element inside %2 element.")
                    .arg(QStringLiteral("Count")).arg(QStringLiteral("ConflictResults")));
                return;
            }

            bool ok;
            mConflictCount = reader.readElementText().toUInt(&ok);

            if (!ok) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - invalid %1 element.")
                    .arg(QStringLiteral("ConflictResults/Value")));
            }
            // Finish the Value element.
            reader.skipCurrentElement();
            // Finish the ConflictResults element.
            reader.skipCurrentElement();
        }
        else if (!readResponseElement(reader)) {
            setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
            return;
        }
    }
}

bool EwsUpdateItemRequest::Update::write(QXmlStreamWriter &writer, EwsItemType itemType) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, updateTypeElementNames[mType]);

    mField.write(writer);

    if (mType != Delete) {
        writer.writeStartElement(ewsTypeNsUri, itemTypeNames[itemType]);
        retVal = mField.writeValue(writer, mValue);
        writer.writeEndElement();
    }

    writer.writeEndElement();

    return retVal;
}

bool EwsUpdateItemRequest::ItemChange::write(QXmlStreamWriter &writer) const
{
    bool retVal = true;

    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("ItemChange"));

    mId.writeItemIds(writer);

    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("Updates"));

    Q_FOREACH(const QSharedPointer<const Update> upd, mUpdates) {
        if (!upd->write(writer, mType)) {
            retVal = false;
            break;
        }
    }

    writer.writeEndElement();

    writer.writeEndElement();

    return retVal;
}
