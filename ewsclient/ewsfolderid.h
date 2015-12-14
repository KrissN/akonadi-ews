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

#ifndef EWSFOLDERID_H
#define EWSFOLDERID_H

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "ewstypes.h"

/**
 *  @brief  EWS Folder Id wrapper class
 *
 *  This class wraps an EWS folder identifier.
 *
 *  In the EWS world a folder id can come in two forms:
 *   - An "actual" folder id identified by a unique, server-generated string (actually it's a
 *     base64-encoded internal server structure). Optionally this id is accompanied by a change
 *     key, which acts as a version number of the folder. Each time something changes with the
 *     folder (either the folder itself or it's content - not sure if this applies to subfolders)
 *     the change key is updated. This gives you access to an older version of the folder and
 *     allows to quickly find out if the folder needs synchronizing.
 *   - A "distinguished" folder id which is a string identifying a list of known root folders
 *     such as 'inbox'. This is necessary for the initial query as there is no way to know the
 *     real folder ids beforehand.
 */
class EwsFolderId
{
public:
    enum Type {
        Distinguished,
        Real,
        Unspecified
    };
    EwsFolderId(EwsDistinguishedId did) : mType(Distinguished), mDid(did) {};
    EwsFolderId(QString id, QString changeKey) : mType(Real), mId(id), mChangeKey(changeKey),
                    mDid(EwsDIdCalendar) {};
    EwsFolderId(const EwsFolderId &id) { *this = id; };
    EwsFolderId(EwsFolderId &&id) { *this = std::move(id); };
    EwsFolderId() : mType(Unspecified), mDid(EwsDIdCalendar) {};

    Type type() const { return mType; };
    QString id() const { return mId; };
    QString changeKey() const { return mChangeKey; };
    EwsDistinguishedId distinguishedId() const { return mDid; };

    EwsFolderId& operator=(const EwsFolderId &other);
    EwsFolderId& operator=(EwsFolderId &&other);
    bool operator==(const EwsFolderId &other) const;

    void writeFolderIds(QXmlStreamWriter &writer) const;
private:
    Type mType;
    QString mId;
    QString mChangeKey;
    EwsDistinguishedId mDid;
};

#endif
