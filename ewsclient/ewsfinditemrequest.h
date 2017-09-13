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
    EwsFindItemRequest(EwsClient &client, QObject *parent);
    virtual ~EwsFindItemRequest();

    void setFolderId(const EwsId &id);
    void setItemShape(const EwsItemShape &shape);
    void setTraversal(EwsTraversalType traversal)
    {
        mTraversal = traversal;
    };
    void setPagination(EwsIndexedViewBasePoint basePoint, unsigned offset, int maxItems = -1)
    {
        mFractional = false;
        mMaxItems = maxItems;
        mPageBasePoint = basePoint;
        mPageOffset = offset;
        mPagination = true;
    }
    void setFractional(unsigned numerator, unsigned denominator, int maxItems = -1)
    {
        mPagination = false;
        mMaxItems = maxItems;
        mFracNumerator = numerator;
        mFracDenominator = denominator;
        mFractional = true;
    }

    virtual void start() override;

    bool includesLastItem() const
    {
        return mIncludesLastItem;
    };
    int nextOffset() const
    {
        return mNextOffset;
    };
    int nextNumerator() const
    {
        return mNextNumerator;
    };
    int nextDenominator() const
    {
        return mNextDenominator;
    };

    const QList<EwsItem> items() const
    {
        return mItems;
    };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    EwsId mFolderId;
    EwsItemShape mShape;
    EwsTraversalType mTraversal;
    bool mPagination;
    EwsIndexedViewBasePoint mPageBasePoint;
    unsigned mPageOffset;
    bool mFractional;
    int mMaxItems;
    unsigned mFracNumerator;
    unsigned mFracDenominator;
    QList<EwsItem> mItems;
    unsigned mTotalItems;
    int mNextOffset;
    int mNextNumerator;
    int mNextDenominator;
    bool mIncludesLastItem;
};

#endif
