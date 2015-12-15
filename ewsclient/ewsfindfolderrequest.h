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

#ifndef EWSFINDFOLDERREQUEST_H
#define EWSFINDFOLDERREQUEST_H

#include "ewsfolderbase.h"
#include "ewsrequest.h"
#include "ewstypes.h"
#include "ewsfoldershape.h"

class EwsFindFolderRequest : public EwsRequest
{
    Q_OBJECT
public:
    EwsFindFolderRequest(EwsClient* parent);
    virtual ~EwsFindFolderRequest();

    void setParentFolderId(const EwsFolderId &id);
    void setFolderShape(const EwsFolderShape &shape);
    void setTraversal(EwsTraversalType traversal) { mTraversal = traversal; };

    virtual void send();

    const QList<QPointer<EwsFolderBase>> folders() const { return mFolders; };
protected:
    virtual bool parseResult(QXmlStreamReader &reader);
    bool parseFoldersResponse(QXmlStreamReader &reader);
    unsigned readChildFolders(EwsFolderBase *parent, unsigned count, QXmlStreamReader &reader);
    EwsFolderBase* readFolder(QXmlStreamReader &reader);
private:
    EwsFolderId mParentId;
    EwsFolderShape mShape;
    EwsTraversalType mTraversal;
    QList<QPointer<EwsFolderBase>> mFolders;
};

#endif
