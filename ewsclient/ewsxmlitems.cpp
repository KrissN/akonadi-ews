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

#include <QtCore/QString>

#include "ewsxmlitems.h"
#include "ewsfolderid.h"
#include "ewsclient_debug.h"

const QString EwsXmlItemBase::soapEnvNsUri = QStringLiteral("http://schemas.xmlsoap.org/soap/envelope/");
const QString EwsXmlItemBase::ewsMsgNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/messages");
const QString EwsXmlItemBase::ewsTypeNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/types");

EwsXmlItemBase::EwsXmlItemBase()
{
}

EwsXmlItemBase::~EwsXmlItemBase()
{
}

void EwsXmlItemBase::write(QXmlStreamWriter &writer) const
{
    // Default implementation does nothing. It's useful for elements which only exist in responses
    // and therefore will never be written.
}

bool EwsXmlItemBase::read(QXmlStreamReader &reader)
{
    // Default implementation does nothing. It's useful for elements which only exist in requests
    // and therefore will never be read.
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS BaseShape element (write only)
//
EwsBaseShapeItem::EwsBaseShapeItem(EwsFolderShape shape)
    : mShape(shape)
{
};

EwsBaseShapeItem::~EwsBaseShapeItem()
{
}

void EwsBaseShapeItem::write(QXmlStreamWriter &writer) const
{
    static const QString shapeNames[] = {
        QStringLiteral("IdOnly"),
        QStringLiteral("Default"),
        QStringLiteral("AllProperties")
    };

    writer.writeTextElement(ewsTypeNsUri, QStringLiteral("BaseShape"), shapeNames[mShape]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS FolderShape element (write only)
//
EwsFolderShapeItem::EwsFolderShapeItem()
    : mBaseShape(0)
{
}

EwsFolderShapeItem::~EwsFolderShapeItem()
{
    delete mBaseShape;
}

void EwsFolderShapeItem::setBaseShape(EwsBaseShapeItem *baseShape)
{
    delete mBaseShape;
    mBaseShape = baseShape;
}

void EwsFolderShapeItem::setBaseShape(EwsFolderShape shape)
{
    delete mBaseShape;
    mBaseShape = new EwsBaseShapeItem(shape);
}

void EwsFolderShapeItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderShape"));
    if (mBaseShape) {
        mBaseShape->write(writer);
    }
    writer.writeEndElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS FolderId element (read-write)
//
EwsFolderIdItem::EwsFolderIdItem()
{
}

EwsFolderIdItem::EwsFolderIdItem(QString id, QString changeKey)
    : mId(id), mChangeKey(changeKey)
{
};

EwsFolderIdItem::~EwsFolderIdItem()
{
}

void EwsFolderIdItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("FolderId"));
    writer.writeAttribute(QStringLiteral("Id"), mId);
    writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
    writer.writeEndElement();
}

bool EwsFolderIdItem::read(QXmlStreamReader &reader)
{
    const QXmlStreamAttributes &attrs = reader.attributes();
    QStringRef ref = attrs.value(QStringLiteral("Id"));
    if (ref.isNull()) {
        qCWarning(EWSCLIENT_LOG) << "Missing Id attribute in FolderId element.";
        return false;
    }
    mId = ref.toString();

    ref = attrs.value(QStringLiteral("ChangeKey"));
    if (ref.isNull()) {
        qCWarning(EWSCLIENT_LOG) << "Missing ChangeKey attribute in FolderId element.";
        return false;
    }
    mChangeKey = ref.toString();

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS DistinguishedFolderId element (write only)
//
EwsDistinguishedFolderIdItem::EwsDistinguishedFolderIdItem(EwsDistinguishedId id)
    : mId(id)
{
};

EwsDistinguishedFolderIdItem::~EwsDistinguishedFolderIdItem()
{
}

void EwsDistinguishedFolderIdItem::write(QXmlStreamWriter &writer) const
{
    static const QString idNames[] = {
        QStringLiteral("calendar"),
        QStringLiteral("contacts"),
        QStringLiteral("deletedItems"),
        QStringLiteral("drafts"),
        QStringLiteral("inbox"),
        QStringLiteral("journal"),
        QStringLiteral("notes"),
        QStringLiteral("outbox"),
        QStringLiteral("sentitems"),
        QStringLiteral("tasks"),
        QStringLiteral("msgfolderroot"),
        QStringLiteral("root"),
        QStringLiteral("junkemail"),
        QStringLiteral("searchfolders"),
        QStringLiteral("voicemail"),
        QStringLiteral("recoverableitemsroot"),
        QStringLiteral("recoverableitemsdeletions"),
        QStringLiteral("recoverableitemsversions"),
        QStringLiteral("recoverableitemspurges"),
        QStringLiteral("archiveroot"),
        QStringLiteral("archivemsgfolderroot"),
        QStringLiteral("archivedeleteditems"),
        QStringLiteral("archiverecoverableitemsroot"),
        QStringLiteral("archiverecoverableitemsdeletions"),
        QStringLiteral("archiverecoverableitemsversions"),
        QStringLiteral("archiverecoverableitemspurges")
    };
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("DistinguishedFolderId"));
    writer.writeAttribute(QStringLiteral("Id"), idNames[mId]);
    writer.writeEndElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS FolderIds element (write only)
//
EwsFolderIdsItem::EwsFolderIdsItem()
    : mFolderId(0), mDistinguishedFolderId(0)
{
};

EwsFolderIdsItem::~EwsFolderIdsItem()
{
    delete mFolderId;
    delete mDistinguishedFolderId;
}

void EwsFolderIdsItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderIds"));
    if (mFolderId) {
        mFolderId->write(writer);
    }
    else if (mDistinguishedFolderId) {
        mDistinguishedFolderId->write(writer);
    }
    writer.writeEndElement();
}

void EwsFolderIdsItem::setFolderId(EwsFolderIdItem *folderId)
{
    delete mFolderId;
    delete mDistinguishedFolderId;
    mDistinguishedFolderId = 0;
    mFolderId = folderId;
}

void EwsFolderIdsItem::setFolderId(QString id, QString changeKey)
{
    delete mFolderId;
    delete mDistinguishedFolderId;
    mDistinguishedFolderId = 0;
    mFolderId = new EwsFolderIdItem(id, changeKey);
}

void EwsFolderIdsItem::setDistinguishedFolderId(EwsDistinguishedFolderIdItem *distinguishedFolderId)
{
    delete mFolderId;
    delete mDistinguishedFolderId;
    mFolderId = 0;
    mDistinguishedFolderId = distinguishedFolderId;
}

void EwsFolderIdsItem::setDistinguishedFolderId(EwsDistinguishedId id)
{
    delete mFolderId;
    delete mDistinguishedFolderId;
    mFolderId = 0;
    mDistinguishedFolderId= new EwsDistinguishedFolderIdItem(id);
}

void EwsFolderIdsItem::setId(const EwsFolderId &id)
{
    if (id.type() == EwsFolderId::Distinguished) {
        setDistinguishedFolderId(id.distinguishedId());
    }
    else {
        setFolderId(id.id(), id.changeKey());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS GetFolder element (write only)
//
EwsGetFolderItem::EwsGetFolderItem()
    : mFolderIds(0), mFolderShape(0)
{
};

EwsGetFolderItem::~EwsGetFolderItem()
{
    delete mFolderIds;
    delete mFolderShape;
}

void EwsGetFolderItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetFolder"));
    // Note: the order of the items matters
    if (mFolderShape) {
        mFolderShape->write(writer);
    }
    if (mFolderIds) {
        mFolderIds->write(writer);
    }
    writer.writeEndElement();
}

void EwsGetFolderItem::setFolderIds(EwsFolderIdsItem *folderIds)
{
    delete mFolderIds;
    mFolderIds = folderIds;
}

void EwsGetFolderItem::setFolderShape(EwsFolderShapeItem *folderShape)
{
    delete mFolderShape;
    mFolderShape = folderShape;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS Response Message base class
//
EwsResponseMessageBase::EwsResponseMessageBase()
    : mResponseClass(EwsResponseSuccess)
{
}

EwsResponseMessageBase::~EwsResponseMessageBase()
{
}

bool EwsResponseMessageBase::readResponseAttr(const QXmlStreamAttributes &attrs)
{
    static const QString respClasses[] = {
        QStringLiteral("Success"),
        QStringLiteral("Warning"),
        QStringLiteral("Error")
    };

    QStringRef respClassRef = attrs.value(QStringLiteral("ResponseClass"));
    if (respClassRef.isNull()) {
        qCWarning(EWSCLIENT_LOG) << "ResponseClass attribute not found in response element";
        return false;
    }

    int i;
    for (i = 0; i < sizeof(respClasses) / sizeof(respClasses[0]); i++) {
        if (respClassRef == respClasses[i]) {
            mResponseClass = static_cast<EwsResponseClass>(i);
            break;
        }
    }

    return (i < sizeof(respClasses) / sizeof(respClasses[0]));
}

bool EwsResponseMessageBase::readResponseElement(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri) {
        qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in response element:" << reader.namespaceUri();
        return false;
    }
    if (reader.name() == QStringLiteral("ResponseCode")) {
        mResponseCode = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("MessageText")) {
        mMessageText = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("DescriptiveLinkKey")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("MessageXml")) {
        reader.skipCurrentElement();
    }
    else {
        qCWarning(EWSCLIENT_LOG) << "Unknown child in response element:" << reader.qualifiedName();
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS GetFolderResponseMessage element (read only)
//
EwsGetFolderResponseMessageItem::EwsGetFolderResponseMessageItem()
    : mFolders(0)
{
}

EwsGetFolderResponseMessageItem::~EwsGetFolderResponseMessageItem()
{
}

bool EwsGetFolderResponseMessageItem::read(QXmlStreamReader &reader)
{
    if (!readResponseAttr(reader.attributes()))
        return false;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri) {
            qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in response element:"
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("Folders")) {
            mFolders = new EwsFoldersItem();
            if (!mFolders->read(reader))
                return false;
        }
        else {
            if (!readResponseElement(reader))
                return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS Folder base class (read only)
//
EwsFolderItemBase::EwsFolderItemBase()
    : mFolderId(0), mParentFolderId(0), mTotalCount(0), mChildFolderCount(0)
{
}

EwsFolderItemBase::~EwsFolderItemBase()
{
    delete mFolderId;
    delete mParentFolderId;
}

bool EwsFolderItemBase::readFolderElement(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in folder element:" << reader.namespaceUri();
        return false;
    }
    if (reader.name() == QStringLiteral("FolderId")) {
        mFolderId = new EwsFolderIdItem();
        mFolderId->read(reader);
    }
    if (reader.name() == QStringLiteral("ParentFolderId")) {
        mParentFolderId = new EwsFolderIdItem();
        mParentFolderId->read(reader);
    }
    if (reader.name() == QStringLiteral("FolderClass")) {
        mFolderClass = reader.readElementText();
    }
    if (reader.name() == QStringLiteral("DisplayName")) {
        mDisplayName = reader.readElementText();
    }
    if (reader.name() == QStringLiteral("TotalCount")) {
        QString count = reader.readElementText();
        bool ok;
        mTotalCount = count.toInt(&ok);
        if (!ok) {
            qCWarning(EWSCLIENT_LOG) << "Failed to convert TotalCount value: " << count;
            return false;
        }
    }
    if (reader.name() == QStringLiteral("ChildFolderCount")) {
        QString count = reader.readElementText();
        bool ok;
        mChildFolderCount = count.toInt(&ok);
        if (!ok) {
            qCWarning(EWSCLIENT_LOG) << "Failed to convert ChildFolderCount value: " << count;
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ExtendedProperty")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ManagedFolderInformation")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("EffectiveRights")) {
        reader.skipCurrentElement();
    }
    else {
        qCWarning(EWSCLIENT_LOG) << "Unknown child in Folder element:" << reader.qualifiedName();
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS Folder base class (read only)
//
EwsFolderItem::EwsFolderItem()
    : mUnreadCount(0)
{
}

EwsFolderItem::~EwsFolderItem()
{
}

bool EwsFolderItem::read(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in Folder element:"
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("UnreadCount")) {
            QString count = reader.readElementText();
            bool ok;
            mUnreadCount = count.toInt(&ok);
            if (!ok) {
                qCWarning(EWSCLIENT_LOG) << "Failed to convert UnreadCount value: " << count;
                return false;
            }
        }
        else if (reader.name() == QStringLiteral("PermissionSet")) {
            reader.skipCurrentElement();
        }
        else {
            if (!readFolderElement(reader))
                return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS Folders base class (read only)
//
EwsFoldersItem::EwsFoldersItem()
{
}

EwsFoldersItem::~EwsFoldersItem()
{
    foreach (EwsFolderItemBase *it, mFolders) {
        delete it;
    }
}

bool EwsFoldersItem::read(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri) {
            qCWarning(EWSCLIENT_LOG) << "Unexpected namespace in Folders element:"
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("Folder")) {
            EwsFolderItem *folder = new EwsFolderItem();
            if (!folder->read(reader))
                return false;
            mFolders.append(folder);
        }
        else {
            qCWarning(EWSCLIENT_LOG) << "Unknown child in Folders element:" << reader.qualifiedName();
            return false;
        }
    }

    return true;
}
