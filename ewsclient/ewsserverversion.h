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

#ifndef EWSSERVERVERSION_H
#define EWSSERVERVERSION_H

#include <QtCore/QString>

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsServerVersion
{
public:
    enum ServerFeature {
        StreamingSubscription,
        FreeBusyChangedEvent,
    };

    EwsServerVersion() : mMajor(0), mMinor(0), mMajorBuild(0), mMinorBuild(0) {};
    EwsServerVersion(uint major, uint minor, QString name, QString friendlyName)
        : mMajor(major), mMinor(minor), mMajorBuild(0), mMinorBuild(0), mName(name),
          mFriendlyName(friendlyName) {};
    EwsServerVersion(QXmlStreamReader &reader);
    EwsServerVersion(const EwsServerVersion &other)
        : mMajor(other.mMajor), mMinor(other.mMinor), mMajorBuild(other.mMajorBuild),
          mMinorBuild(other.mMinorBuild), mName(other.mName), mFriendlyName(other.mFriendlyName) {};

    EwsServerVersion &operator=(const EwsServerVersion &other)
    {
        mMajor = other.mMajor;
        mMinor = other.mMinor;
        mMajorBuild = other.mMajorBuild;
        mMinorBuild = other.mMinorBuild;
        mName = other.mName;
        mFriendlyName = other.mFriendlyName;
        return *this;
    }

    void writeRequestServerVersion(QXmlStreamWriter &writer) const;

    bool operator>(const EwsServerVersion &other) const
    {
        return (mMajor > other.mMajor) ? true : ((mMinor > other.mMinor) ? true : false);
    }
    bool operator<(const EwsServerVersion &other) const
    {
        return (mMajor < other.mMajor) ? true : ((mMinor < other.mMinor) ? true : false);
    }
    bool operator>=(const EwsServerVersion &other) const
    {
        return !(*this < other);
    }
    bool operator<=(const EwsServerVersion &other) const
    {
        return !(*this > other);
    }
    bool operator==(const EwsServerVersion &other) const
    {
        return (mMajor == other.mMajor) && (mMinor == other.mMinor);
    }
    bool operator!=(const EwsServerVersion &other) const
    {
        return !(*this == other);
    }

    bool supports(ServerFeature feature) const;

    bool isValid() const
    {
        return mMajor != 0;
    };

    uint majorVersion() const { return mMajor; };
    uint minorVersion() const { return mMinor; };
    QString name() const { return mName; };

    static const EwsServerVersion& minSupporting(ServerFeature feature);

    QString toString() const;

    static const EwsServerVersion ewsVersion2007;
    static const EwsServerVersion ewsVersion2007Sp1;
    static const EwsServerVersion ewsVersion2007Sp2;
    static const EwsServerVersion ewsVersion2007Sp3;
    static const EwsServerVersion ewsVersion2010;
    static const EwsServerVersion ewsVersion2010Sp1;
    static const EwsServerVersion ewsVersion2010Sp2;
    static const EwsServerVersion ewsVersion2010Sp3;
    static const EwsServerVersion ewsVersion2013;
    static const EwsServerVersion ewsVersion2016;

private:
    uint mMajor;
    uint mMinor;
    uint mMajorBuild;
    uint mMinorBuild;
    QString mName;
    QString mFriendlyName;
};

QDebug operator<<(const QDebug dbg, const EwsServerVersion &version);

#endif
