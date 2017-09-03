/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef EWSCREATEITEMREQUEST_H
#define EWSCREATEITEMREQUEST_H

#include <QList>
#include <QSharedPointer>

#include "ewsitem.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsCreateItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Response : public EwsRequest::Response
    {
    public:
        const EwsId &itemId() const { return mId; };
    protected:
        Response(QXmlStreamReader &reader);

        EwsId mId;

        friend class EwsCreateItemRequest;
    };

    EwsCreateItemRequest(EwsClient &client, QObject *parent);
    virtual ~EwsCreateItemRequest();

    void setItems(const EwsItem::List &items) { mItems = items; };
    void setMessageDisposition(EwsMessageDisposition disp) { mMessageDisp = disp; };
    void setMeetingDisposition(EwsMeetingDisposition disp) { mMeetingDisp = disp; };
    void setSavedFolderId(const EwsId &id) { mSavedFolderId = id; };

    virtual void start() Q_DECL_OVERRIDE;

    const QList<Response> &responses() const { return mResponses; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) Q_DECL_OVERRIDE;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    EwsItem::List mItems;
    EwsId mSavedFolderId;
    EwsMessageDisposition mMessageDisp;
    EwsMeetingDisposition mMeetingDisp;
    QList<Response> mResponses;
};

#endif
