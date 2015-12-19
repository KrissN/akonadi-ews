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

#include "ewsfolderbase.h"
#include "ewsfolderbase_p.h"

#include "ewsmailfolder.h"
#include "ewscalendarfolder.h"
#include "ewscontactsfolder.h"
#include "ewstasksfolder.h"
#include "ewssearchfolder.h"
#include "ewsgetfolderrequest.h"
#include "ewsclient_debug.h"

EwsFolderBasePrivate::EwsFolderBasePrivate()
    : mType(EwsFolderTypeUnknown), mValid(false), mUpdated(false), mCurrentShape(EwsShapeDefault),
      mTotalCount(0), mChildFolderCount(0)
{
}

EwsFolderBasePrivate::~EwsFolderBasePrivate()
{
    delete mGetFolderReq;
}

EwsFolderBase::EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsFolderId id, EwsClient *parent)
    : QObject(parent), d(priv)
{
    d->mId = id;
}

EwsFolderBase::EwsFolderBase(EwsFolderId id, EwsClient *parent)
    : QObject(parent), d(new EwsFolderBasePrivate())
{
    d->mId = id;
}

EwsFolderBase::EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsClient *parent)
    : QObject(parent), d(priv)
{
}

EwsFolderBase::EwsFolderBase(const EwsFolderBase &other)
    : QObject(other.parent()), d(other.d)
{
}

EwsFolderBase::EwsFolderBase(EwsFolderBase &&other)
    : QObject(other.parent()), d(std::move(other.d))
{
}

EwsFolderBase::~EwsFolderBase()
{
}

void EwsFolderBase::requestFinished()
{
    qCDebug(EWSCLIENT_LOG) << "Folder update finished!";
}

bool EwsFolderBase::isValid() const
{
    return d->mValid;
}

EwsFolderType EwsFolderBase::type() const
{
    return d->mType;
}

EwsFolderId EwsFolderBase::id() const
{
    return d->mId;
}

EwsFolderId EwsFolderBase::parentId() const
{
    return d->mParentId;
}

QString EwsFolderBase::folderClass() const
{
    return d->mFolderClass;
}

QString EwsFolderBase::displayName() const
{
    return d->mDisplayName;
}

unsigned EwsFolderBase::totalCount() const
{
    return d->mTotalCount;
}

unsigned EwsFolderBase::childFolderCount() const
{
    return d->mChildFolderCount;
}

bool EwsFolderBase::readBaseFolderElement(QXmlStreamReader &reader)
{
    d->mProperties.clear();

    if (reader.name() == QStringLiteral("FolderId")) {
        d->mId = EwsFolderId(reader);
        if (d->mId.type() == EwsFolderId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("FolderId"));
            return false;
        }
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ParentFolderId")) {
        d->mParentId = EwsFolderId(reader);
        if (d->mId.type() == EwsFolderId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ParentFolderId"));
            return false;
        }
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("FolderClass")) {
        d->mFolderClass = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("FolderClass"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("DisplayName")) {
        d->mDisplayName = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("DisplayName"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("TotalCount")) {
        bool ok;
        d->mTotalCount = reader.readElementText().toUInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("TotalCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ChildFolderCount")) {
        bool ok;
        d->mChildFolderCount = reader.readElementText().toUInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ChildFolderCount"));
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
    else {
        qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - unknown element: %1.")
                        .arg(reader.name().toString());
        return false;
    }
    return true;
}

EwsMailFolder* EwsFolderBase::toMailFolder()
{
    if (d->mType == EwsFolderTypeMail)
        return new EwsMailFolder(d, qobject_cast<EwsClient*>(parent()));
    else
        return 0;
}

EwsCalendarFolder* EwsFolderBase::toCalendarFolder()
{
    if (d->mType == EwsFolderTypeCalendar)
        return new EwsCalendarFolder(d, qobject_cast<EwsClient*>(parent()));
    else
        return 0;
}

EwsContactsFolder* EwsFolderBase::toContactsFolder()
{
    if (d->mType == EwsFolderTypeContacts)
        return new EwsContactsFolder(d, qobject_cast<EwsClient*>(parent()));
    else
        return 0;
}

EwsTasksFolder* EwsFolderBase::toTasksFolder()
{
    if (d->mType == EwsFolderTypeTasks)
        return new EwsTasksFolder(d, qobject_cast<EwsClient*>(parent()));
    else
        return 0;
}

EwsSearchFolder* EwsFolderBase::toSearchFolder()
{
    if (d->mType == EwsFolderTypeSearch)
        return new EwsSearchFolder(d, qobject_cast<EwsClient*>(parent()));
    else
        return 0;
}

bool EwsFolderBase::update()
{
    if (d->mGetFolderReq) {
        qCWarning(EWSCLIENT_LOG) << QStringLiteral("Update already in progress.");
        return false;
    }

    d->mGetFolderReq = new EwsGetFolderRequest(qobject_cast<EwsClient*>(parent()));
    d->mGetFolderReq->setFolderId(d->mId);
    d->mGetFolderReq->setFolderShape(d->mCurrentShape);

    connect(d->mGetFolderReq.data(), &EwsRequest::finished, this, &EwsFolderBase::requestFinished);

    d->mGetFolderReq->send();

    return true;
}

const QVector<QPointer<EwsFolderBase>> EwsFolderBase::childFolders() const
{
    return d->mChildren;
}

void EwsFolderBase::addChild(EwsFolderBase *child)
{
    if (child->parentFolder() != 0) {
        qCWarning(EWSCLIENT_LOG).noquote()
                        << QStringLiteral("Attempt to add child folder which already has a parent (child: %1)").
                        arg(child->id().id());
    }
    d->mChildren.append(child);
    child->setParentFolder(this);
}

EwsFolderBase* EwsFolderBase::parentFolder() const
{
    return d->mParent.data();
}

void EwsFolderBase::setParentFolder(EwsFolderBase *parent)
{
    d->mParent = parent;
}

bool EwsFolderBase::readExtendedProperty(QXmlStreamReader &reader)
{
    EwsPropertyField prop;
    QString value;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid namespace");
            return false;
        }

        if (reader.name() == QStringLiteral("FieldURI")
            || reader.name() == QStringLiteral("IndexedFieldURI")
            || reader.name() == QStringLiteral("ExtendedFieldURI")) {
            if (!prop.read(reader)) {
                return false;
                reader.skipCurrentElement();
            }
            reader.skipCurrentElement();
        }
        else if (reader.name() == QStringLiteral("Value")) {
            value = reader.readElementText();
        }
        else {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS 1request - unexpected element %1")
                            .arg(reader.qualifiedName().toString());
            reader.skipCurrentElement();
            return false;
        }
    }
    d->mProperties.insert(prop, value);

    return true;
}

QStringRef EwsFolderBase::folderProperty(const EwsPropertyField &prop) const
{
    QHash<EwsPropertyField, QString>::const_iterator it = d->mProperties.find(prop);
    if (it != d->mProperties.cend()) {
        return QStringRef(&it.value());
    }
    else {
        return QStringRef();
    }
}
