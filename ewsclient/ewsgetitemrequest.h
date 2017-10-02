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

#ifndef EWSGETITEMREQUEST_H
#define EWSGETITEMREQUEST_H

#include <QList>

#include "ewsitem.h"
#include "ewsitemshape.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class EwsGetItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Response : public EwsRequest::Response
    {
    public:
        explicit Response(QXmlStreamReader &reader);
        bool parseItems(QXmlStreamReader &reader);
        const EwsItem &item() const
        {
            return mItem;
        };
    private:
        EwsItem mItem;
    };

    EwsGetItemRequest(EwsClient &client, QObject *parent);
    ~EwsGetItemRequest() override;

    void setItemIds(const EwsId::List &ids);
    void setItemShape(const EwsItemShape &shape);

    void start() override;

    const QList<Response> &responses() const
    {
        return mResponses;
    };
protected:
    bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    EwsId::List mIds;
    EwsItemShape mShape;
    QList<Response> mResponses;
};

#endif
