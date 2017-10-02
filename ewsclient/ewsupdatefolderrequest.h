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

#ifndef EWSUPDATEFOLDERREQUEST_H
#define EWSUPDATEFOLDERREQUEST_H

#include <QList>
#include <QSharedPointer>

#include "ewsfolder.h"
#include "ewsfoldershape.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsUpdateFolderRequest : public EwsRequest
{
    Q_OBJECT
public:

    class Update
    {
    public:
        bool write(QXmlStreamWriter &writer, EwsFolderType folderType) const;
    protected:
        enum Type {
            Append = 0,
            Set,
            Delete,
            Unknown
        };

        Update(EwsPropertyField field, const QVariant &val, Type type)
            : mField(field), mValue(val), mType(type) {};

        EwsPropertyField mField;
        QVariant mValue;
        Type mType;
    };

    class AppendUpdate : public Update
    {
    public:
        AppendUpdate(EwsPropertyField field, const QVariant &val) : Update(field, val, Append) {};
    };

    class SetUpdate : public Update
    {
    public:
        SetUpdate(EwsPropertyField field, const QVariant &val) : Update(field, val, Set) {};
    };

    class DeleteUpdate : public Update
    {
    public:
        DeleteUpdate(EwsPropertyField field) : Update(field, QVariant(), Delete) {};
    };

    class FolderChange
    {
    public:
        FolderChange(EwsId folderId, EwsFolderType type) : mId(folderId), mType(type) {};
        void addUpdate(const Update *upd)
        {
            mUpdates.append(QSharedPointer<const Update>(upd));
        }
        bool write(QXmlStreamWriter &writer) const;
    private:
        EwsId mId;
        EwsFolderType mType;
        QList<QSharedPointer<const Update>> mUpdates;
    };

    class Response : public EwsRequest::Response
    {
    public:
        const EwsId &folderId() const
        {
            return mId;
        };
    protected:
        Response(QXmlStreamReader &reader);

        EwsId mId;

        friend class EwsUpdateFolderRequest;
    };

    EwsUpdateFolderRequest(EwsClient &client, QObject *parent);
    ~EwsUpdateFolderRequest() override;

    void addFolderChange(const FolderChange &change)
    {
        mChanges.append(change);
    };

    void start() override;

    const QList<Response> &responses() const
    {
        return mResponses;
    };
protected:
    bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);
private:
    QList<FolderChange> mChanges;
    QList<Response> mResponses;
};

#endif
