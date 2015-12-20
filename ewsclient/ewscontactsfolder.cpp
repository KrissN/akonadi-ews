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
#include "ewscontactsfolder.h"
#include "ewsclient_debug.h"

class EwsContactsFolderPrivate : public EwsFolderBasePrivate
{
public:
    EwsContactsFolderPrivate();
    EwsContactsFolderPrivate(const EwsFolderBasePrivate &other);
};

EwsContactsFolderPrivate::EwsContactsFolderPrivate()
    : EwsFolderBasePrivate()
{
    mType = EwsFolderTypeContacts;
}

EwsContactsFolder::EwsContactsFolder(EwsId id, EwsClient *parent)
    : EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate>(new EwsContactsFolderPrivate()), id, parent)
{
}

EwsContactsFolder::EwsContactsFolder(QXmlStreamReader &reader, EwsClient *parent)
    : EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate>(new EwsContactsFolderPrivate()), parent)
{
    EwsContactsFolderPrivate *d = reinterpret_cast<EwsContactsFolderPrivate*>(this->d.data());

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

EwsContactsFolder::EwsContactsFolder(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsClient *parent)
    : EwsFolderBase(priv, parent)
{
}

EwsContactsFolder::~EwsContactsFolder()
{
}

