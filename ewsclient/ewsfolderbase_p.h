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

#ifndef EWSFOLDERBASE_P_H
#define EWSFOLDERBASE_P_H

#include <QtCore/QSharedData>

#include "ewsfolderid.h"
#include "ewsgetfolderrequest.h"

class EwsFolderBasePrivate : public QSharedData
{
public:
    EwsFolderBasePrivate();
    virtual ~EwsFolderBasePrivate();

    EwsFolderType mType;

    QPointer<EwsGetFolderRequest> mGetFolderReq;

    // When the folder is first constructed it will only contain the id and will therefore be
    // invalid. Once updated through EWS the remaining data will be populated and the folder will
    // be valid.
    bool mValid;

    // Set to 'true' when the folder change key was bumped after the last sync.
    bool mUpdated;

    EwsFolderId mId;

    EwsBaseShape mCurrentShape;

    EwsFolderId mParentId;
    QString mFolderClass;
    QString mDisplayName;
    unsigned mTotalCount;
    unsigned mChildFolderCount;

    QPointer<EwsFolderBase> mParent;
    QVector<QPointer<EwsFolderBase>> mChildren;

    QHash<EwsPropertyField, QString> mProperties;
};

#endif
