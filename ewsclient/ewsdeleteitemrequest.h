/*
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

#ifndef EWSDELETEITEMREQUEST_H
#define EWSDELETEITEMREQUEST_H

#include <QList>

#include "ewsitem.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;

class EwsDeleteItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    enum Type {
        HardDelete = 0,
        SoftDelete,
        MoveToDeletedItems
    };

    class Response : public EwsRequest::Response
    {
    public:
    protected:
        Response(QXmlStreamReader &reader);

        friend class EwsDeleteItemRequest;
    };

    EwsDeleteItemRequest(EwsClient &client, QObject *parent);
    virtual ~EwsDeleteItemRequest();

    void setItemIds(const EwsId::List &ids) { mIds = ids; };
    void setType(Type type) { mType = type; };

    virtual void start() override;

    const QList<Response> &responses() const { return mResponses; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    EwsId::List mIds;
    Type mType;
    QList<Response> mResponses;
};

#endif
