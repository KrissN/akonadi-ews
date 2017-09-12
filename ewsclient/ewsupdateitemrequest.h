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

#ifndef EWSUPDATEITEMREQUEST_H
#define EWSUPDATEITEMREQUEST_H

#include <QList>
#include <QSharedPointer>

#include "ewsitem.h"
#include "ewsitemshape.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsUpdateItemRequest : public EwsRequest
{
    Q_OBJECT
public:

    class Update
    {
    public:
        bool write(QXmlStreamWriter &writer, EwsItemType itemType) const;
    protected:
        enum Type {
            Append = 0,
            Set,
            Delete,
            Unknown
        };

        Update(EwsPropertyField field, QVariant val, Type type)
            : mField(field), mValue(val), mType(type) {};

        EwsPropertyField mField;
        QVariant mValue;
        Type mType;
    };

    class AppendUpdate : public Update
    {
    public:
        AppendUpdate(EwsPropertyField field, QVariant val) : Update(field, val, Append) {};
    };

    class SetUpdate : public Update
    {
    public:
        SetUpdate(EwsPropertyField field, QVariant val) : Update(field, val, Set) {};
    };

    class DeleteUpdate : public Update
    {
    public:
        DeleteUpdate(EwsPropertyField field) : Update(field, QVariant(), Delete) {};
    };

    class ItemChange
    {
    public:
        ItemChange(EwsId itemId, EwsItemType type) : mId(itemId), mType(type) {};
        void addUpdate(const Update *upd) {
            mUpdates.append(QSharedPointer<const Update>(upd));
        }
        bool write(QXmlStreamWriter &writer) const;
    private:
        EwsId mId;
        EwsItemType mType;
        QList<QSharedPointer<const Update>> mUpdates;
    };

    class Response : public EwsRequest::Response
    {
    public:
        const EwsId &itemId() const { return mId; };
        unsigned conflictCount() const { return mConflictCount; };
    protected:
        Response(QXmlStreamReader &reader);

        unsigned mConflictCount;
        EwsId mId;

        friend class EwsUpdateItemRequest;
    };

    EwsUpdateItemRequest(EwsClient &client, QObject *parent);
    virtual ~EwsUpdateItemRequest();

    void addItemChange(const ItemChange &change) { mChanges.append(change); };
    void setMessageDisposition(EwsMessageDisposition disp) { mMessageDisp = disp; };
    void setConflictResolution(EwsConflictResolution resol) { mConflictResol = resol; };
    void setMeetingDisposition(EwsMeetingDisposition disp) { mMeetingDisp = disp; };
    void setSavedFolderId(const EwsId &id) { mSavedFolderId = id; };

    virtual void start() override;

    const QList<Response> &responses() const { return mResponses; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    QList<ItemChange> mChanges;
    EwsMessageDisposition mMessageDisp;
    EwsConflictResolution mConflictResol;
    EwsMeetingDisposition mMeetingDisp;
    EwsId mSavedFolderId;
    QList<Response> mResponses;
};

#endif
