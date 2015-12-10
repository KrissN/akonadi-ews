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

#include "ewstypes.h"

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
    EwsFolderId() : mType(Unspecified), mDid(EwsDIdCalendar) {};

    Type type() const { return mType; };
    QString id() const { return mId; };
    QString changeKey() const { return mChangeKey; };
    EwsDistinguishedId distinguishedId() const { return mDid; };

    EwsFolderId& operator=(const EwsFolderId &other)
    {
        mType = other.mType;
        if (mType == Distinguished) {
            mDid = other.mDid;
        }
        else if (mType == Real) {
            mId = other.mId;
            mChangeKey = other.mChangeKey;
        }
        return *this;
    }

    bool operator==(const EwsFolderId &other) const
    {
        if (mType != other.mType)
            return false;

        if (mType == Distinguished) {
            return (mDid == other.mDid);
        }
        else if (mType == Real) {
            return (mId == other.mId && mChangeKey == other.mChangeKey);
        }
        return true;
    }
private:
    Type mType;
    QString mId;
    QString mChangeKey;
    EwsDistinguishedId mDid;
};

#endif
