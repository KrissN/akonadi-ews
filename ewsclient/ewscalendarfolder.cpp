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

#include "ewsfolderbase_p.h"
#include "ewscalendarfolder.h"
#include "ewsclient_debug.h"

class EwsCalendarFolderPrivate : public EwsFolderBasePrivate
{
public:
    EwsCalendarFolderPrivate();
    EwsCalendarFolderPrivate(const EwsFolderBasePrivate &other);
};

EwsCalendarFolderPrivate::EwsCalendarFolderPrivate()
    : EwsFolderBasePrivate()
{
    mType = EwsFolderTypeCalendar;
}

EwsCalendarFolder::EwsCalendarFolder(EwsId id, EwsClient *parent)
    : EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate>(new EwsCalendarFolderPrivate()), id, parent)
{
}

EwsCalendarFolder::EwsCalendarFolder(QXmlStreamReader &reader, EwsClient *parent)
    : EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate>(new EwsCalendarFolderPrivate()), parent)
{
    EwsCalendarFolderPrivate *d = reinterpret_cast<EwsCalendarFolderPrivate*>(this->d.data());

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in Folder element:"
                            << reader.namespaceUri();
            return;
        }

        if (reader.name() == QStringLiteral("PermissionSet")) {
            // Unsupported - ignore
        }
        else if (!readBaseFolderElement(reader)) {
            qCWarning(EWSCLIENT_LOG).noquote() << QStringLiteral("Invalid Folder child: %1").arg(reader.qualifiedName().toString());
            return;
        }
    }
    d->mValid = true;
}

EwsCalendarFolder::EwsCalendarFolder(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsClient *parent)
    : EwsFolderBase(priv, parent)
{
}

EwsCalendarFolder::~EwsCalendarFolder()
{
}

