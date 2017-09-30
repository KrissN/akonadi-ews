/*
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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewsitembase_p.h"
#include "ewsxml.h"
#include "ewseffectiverights.h"
#include "ewsclient_debug.h"

#define D_PTR EwsFolderPrivate *d = reinterpret_cast<EwsFolderPrivate*>(this->d.data());
#define D_CPTR const EwsFolderPrivate *d = reinterpret_cast<const EwsFolderPrivate*>(this->d.data());

static const QVector<QString> folderTypeNames = {
    QStringLiteral("Folder"),
    QStringLiteral("CalendarFolder"),
    QStringLiteral("ContactsFolder"),
    QStringLiteral("SearchFolder"),
    QStringLiteral("TasksFolder")
};

class EwsFolderPrivate : public EwsItemBasePrivate
{
public:
    typedef EwsXml<EwsItemFields> XmlProc;

    EwsFolderPrivate();
    EwsFolderPrivate(const EwsItemBasePrivate &other);
    virtual EwsItemBasePrivate *clone() const override
    {
        return new EwsFolderPrivate(*this);
    }

    static bool effectiveRightsReader(QXmlStreamReader &reader, QVariant &val);

    EwsFolderType mType;
    EwsFolder *mParent;
    QVector<EwsFolder> mChildren;
    static const XmlProc mStaticEwsXml;
    XmlProc mEwsXml;
};

typedef EwsXml<EwsItemFields> ItemFieldsReader;

static const QVector<EwsFolderPrivate::XmlProc::Item> ewsFolderItems = {
    {EwsFolderFieldFolderId, QStringLiteral("FolderId"), &ewsXmlIdReader, &ewsXmlIdWriter},
    {EwsFolderFieldParentFolderId, QStringLiteral("ParentFolderId"), &ewsXmlIdReader},
    {EwsFolderFieldFolderClass, QStringLiteral("FolderClass"), &ewsXmlTextReader, &ewsXmlTextWriter},
    {EwsFolderFieldDisplayName, QStringLiteral("DisplayName"), &ewsXmlTextReader, &ewsXmlTextWriter},
    {EwsFolderFieldTotalCount, QStringLiteral("TotalCount"), &ewsXmlUIntReader, &ewsXmlUIntWriter},
    {EwsFolderFieldChildFolderCount, QStringLiteral("ChildFolderCount"), &ewsXmlUIntReader},
    {EwsItemFieldExtendedProperties, QStringLiteral("ExtendedProperty"),
        &EwsItemBasePrivate::extendedPropertyReader, &EwsItemBasePrivate::extendedPropertyWriter},
    {EwsFolderFieldUnreadCount, QStringLiteral("UnreadCount"), &ewsXmlUIntReader},
    {EwsFolderPrivate::XmlProc::Ignore, QStringLiteral("SearchParameters")},
    {EwsFolderFieldEffectiveRights, QStringLiteral("EffectiveRights"),
        &EwsFolderPrivate::effectiveRightsReader},
    {EwsFolderPrivate::XmlProc::Ignore, QStringLiteral("ManagedFolderInformation")},
};

const EwsFolderPrivate::XmlProc EwsFolderPrivate::mStaticEwsXml(ewsFolderItems);

EwsFolderPrivate::EwsFolderPrivate()
    : EwsItemBasePrivate(), mType(EwsFolderTypeUnknown), mParent(0), mEwsXml(mStaticEwsXml)
{
}

bool EwsFolderPrivate::effectiveRightsReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsEffectiveRights rights(reader);
    if (!rights.isValid()) {
        reader.skipCurrentElement();
        return false;
    }
    val = QVariant::fromValue<EwsEffectiveRights>(rights);
    return true;
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
    uint i = 0;
    d->mType = EwsFolderTypeUnknown;
    Q_FOREACH (const QString &name, folderTypeNames) {
        if (name == reader.name()) {
            d->mType = static_cast<EwsFolderType>(i);
            break;
        }
        i++;
    }
    if (d->mType == EwsFolderTypeUnknown) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unknown folder type %1").arg(reader.name().toString());
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << "Unexpected namespace in folder element:"
                                    << reader.namespaceUri();
            return;
        }

        if (!readBaseFolderElement(reader)) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Invalid folder child: %1").arg(reader.qualifiedName().toString());
            return;
        }
    }
    d->mValid = true;

}

EwsFolder::EwsFolder(const EwsFolder &other)
    : EwsItemBase(other.d)
{
    qRegisterMetaType<EwsEffectiveRights>();
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

void EwsFolder::setType(EwsFolderType type)
{
    D_PTR
    d->mType = type;
}

bool EwsFolder::readBaseFolderElement(QXmlStreamReader &reader)
{
    D_PTR

    if (!d->mEwsXml.readItem(reader, QStringLiteral("Folder"), ewsTypeNsUri)) {
        return false;
    }

    d->mFields = d->mEwsXml.values();

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

    if (child.parentFolder() != nullptr) {
        qCWarning(EWSCLI_LOG).noquote()
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

EwsFolder &EwsFolder::operator=(const EwsFolder &other)
{
    d = other.d;
    return *this;
}

EwsFolder &EwsFolder::operator=(EwsFolder &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsFolder::write(QXmlStreamWriter &writer) const
{
    D_CPTR

    writer.writeStartElement(ewsTypeNsUri, folderTypeNames[d->mType]);

    bool status = d->mEwsXml.writeItems(writer, folderTypeNames[d->mType], ewsTypeNsUri, d->mFields);

    writer.writeEndElement();

    return status;
}

