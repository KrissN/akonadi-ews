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
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS BaseShape element (write only)
//
EwsBaseShapeItem::EwsBaseShapeItem(Shape shape)
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

void EwsFolderShapeItem::setBaseShape(EwsBaseShapeItem::Shape shape)
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
// EWS FolderId element (write only)
//
EwsFolderIdItem::EwsFolderIdItem(QString id, QString changeKey)
    : mId(id), mChangeKey(changeKey)
{
};

EwsFolderIdItem::~EwsFolderIdItem()
{
}

void EwsFolderIdItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("BaseShape"));
    writer.writeAttribute(QStringLiteral("Id"), mId);
    writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
    writer.writeEndElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EWS DistinguishedFolderId element (write only)
//
EwsDistinguishedFolderIdItem::EwsDistinguishedFolderIdItem(DistinguishedId id)
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
    writer.writeTextElement(ewsTypeNsUri, QStringLiteral("DistinguishedFolderId"), idNames[mId]);
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

void EwsFolderIdsItem::setDistinguishedFolderId(EwsDistinguishedFolderIdItem::DistinguishedId id)
{
    delete mFolderId;
    delete mDistinguishedFolderId;
    mFolderId = 0;
    mDistinguishedFolderId= new EwsDistinguishedFolderIdItem(id);
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
}

void EwsGetFolderItem::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetFolder"));
    if (mFolderIds) {
        mFolderIds->write(writer);
    }
    if (mFolderShape) {
        mFolderShape->write(writer);
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
EwsResponseMessageItem::EwsResponseMessageItem()
    : mResponseClass(Success)
{
}

EwsResponseMessageItem::~EwsResponseMessageItem()
{
}

bool EwsResponseMessageItem::readResponseAttr(const QXmlStreamAttributes &attrs)
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
        if (respClassRef == respClasses[i])
            mResponseClass = static_cast<ResponseClass>(i);
    }

    return (i < sizeof(respClasses) / sizeof(respClasses[0]));
}

bool EwsResponseMessageItem::readResponseElement(QXmlStreamReader &reader)
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

        }
        else {
            if (!readResponseElement(reader))
                return false;
        }
    }

    return true;
}

