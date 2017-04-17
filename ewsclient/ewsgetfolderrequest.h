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

#ifndef EWSGETFOLDERREQUEST_H
#define EWSGETFOLDERREQUEST_H

#include "ewsfolder.h"
#include "ewsrequest.h"
#include "ewstypes.h"
#include "ewsfoldershape.h"

class EwsGetFolderRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Response : public EwsRequest::Response
    {
    public:
        Response(QXmlStreamReader &reader);
        bool parseFolders(QXmlStreamReader &reader);

        const EwsFolder &folder() const { return mFolder; };
    private:
        EwsFolder mFolder;
    };

    EwsGetFolderRequest(EwsClient &client, QObject *parent);
    virtual ~EwsGetFolderRequest();

    void setFolderIds(const EwsId::List &ids);
    void setFolderShape(const EwsFolderShape &shape);

    virtual void start() Q_DECL_OVERRIDE;

    const QList<Response> &responses() const { return mResponses; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader) Q_DECL_OVERRIDE;
    bool parseFoldersResponse(QXmlStreamReader &reader);
private:
    EwsId::List mIds;
    EwsFolderShape mShape;
    QList<Response> mResponses;
};

#endif
