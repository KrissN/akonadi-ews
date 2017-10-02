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

#ifndef EWSSYNCFOLDERITEMSREQUEST_H
#define EWSSYNCFOLDERITEMSREQUEST_H

#include "ewsitem.h"
#include "ewsrequest.h"
#include "ewstypes.h"
#include "ewsitemshape.h"

class EwsSyncFolderItemsRequest : public EwsRequest
{
    Q_OBJECT
public:
    enum ChangeType {
        Create,
        Update,
        Delete,
        ReadFlagChange,
        Unknown
    };

    class Response;

    class Change
    {
    public:
        typedef QList<Change> List;

        ChangeType type() const
        {
            return mType;
        };
        const EwsId &itemId() const
        {
            return mId;
        };
        const EwsItem &item() const
        {
            return mItem;
        };
        bool isRead() const
        {
            return mIsRead;
        };
    protected:
        Change(QXmlStreamReader &reader);
        bool isValid() const
        {
            return mType != Unknown;
        };

        ChangeType mType;
        EwsId mId;
        EwsItem mItem;
        bool mIsRead;

        friend class Response;
    };

    EwsSyncFolderItemsRequest(EwsClient &client, QObject *parent);
    ~EwsSyncFolderItemsRequest() override;

    void setFolderId(const EwsId &id);
    void setItemShape(const EwsItemShape &shape);
    void setSyncState(const QString &state);
    void setMaxChanges(uint max);

    void start() override;

    bool includesLastItem() const
    {
        return mIncludesLastItem;
    };

    const Change::List &changes() const
    {
        return mChanges;
    };
    const QString &syncState() const
    {
        return mSyncState;
    };
protected:
    bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:

    EwsId mFolderId;
    EwsItemShape mShape;
    QString mSyncState;
    uint mMaxChanges;
    Change::List mChanges;
    bool mIncludesLastItem;
};

Q_DECLARE_METATYPE(EwsSyncFolderItemsRequest::Change::List)

#endif
