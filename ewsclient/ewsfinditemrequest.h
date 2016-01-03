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

#ifndef EWSFINDITEMREQUEST_H
#define EWSFINDITEMREQUEST_H

#include "ewsitem.h"
#include "ewsrequest.h"
#include "ewstypes.h"
#include "ewsitemshape.h"

class EwsFindItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    EwsFindItemRequest(EwsClient& client, QObject *parent);
    virtual ~EwsFindItemRequest();

    void setFolderId(const EwsId &id);
    void setItemShape(const EwsItemShape &shape);
    void setTraversal(EwsTraversalType traversal) { mTraversal = traversal; };
    void setPagination(unsigned pageSize, unsigned page);

    virtual void start() Q_DECL_OVERRIDE;

    const QList<EwsItem> items() const { return mItems; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) Q_DECL_OVERRIDE;
    bool parseItemsResponse(QXmlStreamReader &reader, EwsResponseClass responseClass);
    EwsItem* readItem(QXmlStreamReader &reader);
private:
    EwsId mFolderId;
    EwsItemShape mShape;
    EwsTraversalType mTraversal;
    unsigned mPageSize;
    unsigned mPage;
    QList<EwsItem> mItems;
};

#endif
