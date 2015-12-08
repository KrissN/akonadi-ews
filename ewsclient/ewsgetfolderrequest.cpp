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

#include <QtCore/QXmlStreamWriter>

#include "ewsgetfolderrequest.h"

EwsGetFolderRequest::EwsGetFolderRequest(EwsClient* parent)
    : EwsRequest(parent), mGetFolderItem(new EwsGetFolderItem())
{
}

EwsGetFolderRequest::~EwsGetFolderRequest()
{
    delete mGetFolderItem;
}

void EwsGetFolderRequest::setFolderId(QString id, QString changeKey)
{
    if (!mGetFolderItem->folderIds()) {
        mGetFolderItem->setFolderIds(new EwsFolderIdsItem());
    }
    mGetFolderItem->folderIds()->setFolderId(id, changeKey);
}

void EwsGetFolderRequest::setDistinguishedFolderId(EwsDistinguishedFolderIdItem::DistinguishedId id)
{
    if (!mGetFolderItem->folderIds()) {
        mGetFolderItem->setFolderIds(new EwsFolderIdsItem());
    }
    mGetFolderItem->folderIds()->setDistinguishedFolderId(id);
}

void EwsGetFolderRequest::setFolderShape(EwsBaseShapeItem::Shape shape)
{
    if (!mGetFolderItem->folderShape()) {
        mGetFolderItem->setFolderShape(new EwsFolderShapeItem());
    }
    mGetFolderItem->folderShape()->setBaseShape(shape);
}

void EwsGetFolderRequest::send()
{
    prepare(mGetFolderItem);

    doSend();
}
