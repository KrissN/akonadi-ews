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

#include "ewsserverversion.h"

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "ewstypes.h"
#include "ewsclient_debug.h"

const EwsServerVersion EwsServerVersion::ewsVersion2007(8, 0, QStringLiteral("Exchange2007"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp1(8, 1, QStringLiteral("Exchange2007_SP1"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp2(8, 2, QStringLiteral("Exchange2007_SP2"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp3(8, 3, QStringLiteral("Exchange2007_SP3"));
const EwsServerVersion EwsServerVersion::ewsVersion2010(14, 0, QStringLiteral("Exchange2010"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp1(14, 1, QStringLiteral("Exchange2010_SP1"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp2(14, 2, QStringLiteral("Exchange2010_SP2"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp3(14, 3, QStringLiteral("Exchange2010_SP3"));
const EwsServerVersion EwsServerVersion::ewsVersion2013(15, 0, QStringLiteral("Exchange2013"));
const EwsServerVersion EwsServerVersion::ewsVersion2016(15, 1, QStringLiteral("Exchange2016"));

EwsServerVersion::EwsServerVersion(QXmlStreamReader &reader)
    : mMajor(0), mMinor(0)
{
    // <h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" MajorBuildNumber=\"248\"
    // MinorBuildNumber=\"2\" Version=\"Exchange2010_SP2\" />
    QXmlStreamAttributes attrs = reader.attributes();

    QStringRef majorRef = attrs.value(QStringLiteral("MajorVersion"));
    QStringRef minorRef = attrs.value(QStringLiteral("MinorVersion"));
    QStringRef nameRef = attrs.value(QStringLiteral("Version"));

    if (majorRef.isNull() || minorRef.isNull() || nameRef.isNull()) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Error reading server version info - missing attribute.");
        return;
    }

    bool ok;
    uint majorVer = majorRef.toUInt(&ok);
    if (!ok) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Error reading server version info - invalid major version number.");
        return;
    }
    uint minorVer = minorRef.toUInt(&ok);
    if (!ok) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Error reading server version info - invalid minor version number.");
        return;
    }

    mMajor = majorVer;
    mMinor = minorVer;
    mName = nameRef.toString();
}

void EwsServerVersion::writeRequestServerVersion(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("RequestServerVersion"));
    writer.writeAttribute(QStringLiteral("Version"), mName);
    writer.writeEndElement();
}

bool EwsServerVersion::supports(ServerFeature feature) const
{
    switch (feature) {
    case StreamingSubscription:
        return *this >= ewsVersion2010Sp1;
    default:
        return false;
    }
}

QDebug operator<<(QDebug debug, const EwsServerVersion &version)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsServerVersion(");
    if (version.isValid()) {
        d << version.majorVersion() << QStringLiteral(", ") << version.minorVersion()
                        << QStringLiteral(", ") << version.name();
    }
    d << QStringLiteral(")");
    return debug;
}
