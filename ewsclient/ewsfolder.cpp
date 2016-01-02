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

#include "ewsfolder.h"

#include "ewsitembase_p.h"
#include "ewsclient_debug.h"

#define D_PTR EwsFolderPrivate *d = reinterpret_cast<EwsFolderPrivate*>(this->d.data());
#define D_CPTR const EwsFolderPrivate *d = reinterpret_cast<const EwsFolderPrivate*>(this->d.data());

class EwsFolderPrivate : public EwsItemBasePrivate
{
public:
    EwsFolderPrivate();
    EwsFolderPrivate(const EwsItemBasePrivate &other);
    virtual EwsItemBasePrivate *clone() const Q_DECL_OVERRIDE
    {
        return new EwsFolderPrivate(*this);
    }

    EwsFolderType mType;
    EwsFolder *mParent;
    QVector<EwsFolder> mChildren;
};

EwsFolderPrivate::EwsFolderPrivate()
    : EwsItemBasePrivate(), mType(EwsFolderTypeUnknown), mParent(0)
{
}

EwsFolder::EwsFolder()
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsFolderPrivate()))
{
}

EwsFolder::EwsFolder(QXmlStreamReader &reader)
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsFolderPrivate()))
{
    D_PTR

    // Check what item type are we
    if (reader.name() == QStringLiteral("Folder")) {
        d->mType = EwsFolderTypeMail;
    }
    else if (reader.name() == QStringLiteral("CalendarFolder")) {
        d->mType = EwsFolderTypeCalendar;
    }
    else if (reader.name() == QStringLiteral("ContactsFolder")) {
        d->mType = EwsFolderTypeContacts;
    }
    else if (reader.name() == QStringLiteral("TasksFolder")) {
        d->mType = EwsFolderTypeTasks;
    }
    else if (reader.name() == QStringLiteral("SearchFolder")) {
        d->mType = EwsFolderTypeSearch;
    }
    else {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unknown folder type %1").arg(reader.name().toString());
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLIENT_LOG) << "Unexpected namespace in folder element:"
                            << reader.namespaceUri();
            return;
        }

        if (!readBaseFolderElement(reader)) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Invalid folder child: %1").arg(reader.qualifiedName().toString());
            return;
        }
    }
    d->mValid = true;

}

EwsFolder::EwsFolder(const EwsFolder &other)
    : EwsItemBase(other.d)
{
}

EwsFolder::EwsFolder(EwsFolder &&other)
    : EwsItemBase(other.d)
{
}

EwsFolder::~EwsFolder()
{
}

EwsFolderType EwsFolder::type() const
{
    D_CPTR
    return d->mType;
}

bool EwsFolder::readBaseFolderElement(QXmlStreamReader &reader)
{
    if (reader.name() == QStringLiteral("FolderId")) {
        EwsId id = EwsId(reader);
        if (id.type() == EwsId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("FolderId"));
            return false;
        }
        d->mFields[EwsFolderFieldFolderId] = QVariant::fromValue(id);
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ParentFolderId")) {
        EwsId id = EwsId(reader);
        if (id.type() == EwsId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ParentFolderId"));
            return false;
        }
        reader.skipCurrentElement();
        d->mFields[EwsFolderFieldParentFolderId] = QVariant::fromValue(id);
    }
    else if (reader.name() == QStringLiteral("FolderClass")) {
        d->mFields[EwsFolderFieldFolderClass] = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("FolderClass"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("DisplayName")) {
        d->mFields[EwsFolderFieldDisplayName] = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("DisplayName"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("TotalCount")) {
        bool ok;
        d->mFields[EwsFolderFieldTotalCount] = reader.readElementText().toUInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("TotalCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ChildFolderCount")) {
        bool ok;
        d->mFields[EwsFolderFieldChildFolderCount] = reader.readElementText().toUInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ChildFolderCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("UnreadCount")) {
        bool ok;
        d->mFields[EwsFolderFieldUnreadCount] = reader.readElementText().toUInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("UnreadCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ExtendedProperty")) {
        if (!readExtendedProperty(reader)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ManagedFolderInformation")) {
        // Unsupported - ignore
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("EffectiveRights")) {
        // Unsupported - ignore
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("PermissionSet")) {
        // Unsupported - ignore
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("SearchParameters")) {
        // Unsupported - ignore
        reader.skipCurrentElement();
    }
    else {
        qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - unknown element: %1.")
                        .arg(reader.name().toString());
        return false;
    }
    return true;
}

const QVector<EwsFolder> EwsFolder::childFolders() const
{
    D_CPTR
    return d->mChildren;
}

void EwsFolder::addChild(EwsFolder &child)
{
    D_PTR

    if (child.parentFolder() != 0) {
        qCWarning(EWSCLIENT_LOG).noquote()
                        << QStringLiteral("Attempt to add child folder which already has a parent (child: %1)").
                        arg(child[EwsFolderFieldFolderId].value<EwsId>().id());
    }
    d->mChildren.append(child);
    child.setParentFolder(this);
}

EwsFolder* EwsFolder::parentFolder() const
{
    D_CPTR
    return d->mParent;
}

void EwsFolder::setParentFolder(EwsFolder *parent)
{
    D_PTR
    d->mParent = parent;
}


