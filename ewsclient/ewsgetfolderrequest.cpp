/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewsgetfolderrequest.h"
#include "ewsclient_debug.h"

EwsGetFolderRequest::EwsGetFolderRequest(EwsClient* parent)
    : EwsRequest(parent), mGetFolderResponseItem(new EwsGetFolderResponseMessageItem())
{
}

EwsGetFolderRequest::~EwsGetFolderRequest()
{
    delete mGetFolderResponseItem;
}

void EwsGetFolderRequest::setFolderId(const EwsFolderId &id)
{
    mId = id;
}

void EwsGetFolderRequest::setFolderShape(const EwsFolderShape &shape)
{
    mShape = shape;
}

void EwsGetFolderRequest::send()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetFolder"));

    mShape.write(writer);

    mId.writeFolderIds(writer);

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLIENT_LOG) << reqString;

    prepare(reqString);

    doSend();
}

bool EwsGetFolderRequest::parseResult(QXmlStreamReader &reader)
{
    if (reader.name() != QStringLiteral("GetFolderResponse")
        || reader.namespaceUri() != EwsXmlItemBase::ewsMsgNsUri) {
        return setError("Failed to read EWS request - expected GetFolderResponse element.");
    }

    if (!reader.readNextStartElement()) {
        return setError("Failed to read EWS request - expected a child element in GetFolderResponse element.");
    }

    if (reader.name() != QStringLiteral("ResponseMessages")
        || reader.namespaceUri() != EwsXmlItemBase::ewsMsgNsUri) {
        return setError("Failed to read EWS request - expected ResponseMessages element.");
    }

    if (!reader.readNextStartElement()) {
        return setError("Failed to read EWS request - expected a child element in ResponseMessages element.");
    }

    if (reader.name() != QStringLiteral("GetFolderResponseMessage")
        || reader.namespaceUri() != EwsXmlItemBase::ewsMsgNsUri) {
        return setError("Failed to read EWS request - expected GetFolderResponseMessage element.");
    }

    if (!mGetFolderResponseItem->read(reader)) {
        return setError("Failed to read EWS request");
    }


    if (mGetFolderResponseItem->responseClass() != EwsResponseSuccess)
    {
        return setError(QStringLiteral("EWS error: ") + mGetFolderResponseItem->responseCode() +
                        QStringLiteral(": ") + mGetFolderResponseItem->responseMessage());
    }
    else
    {

    }

    return true;
}
