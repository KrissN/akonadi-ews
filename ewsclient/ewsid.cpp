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

#include "ewsid.h"

#include <QtCore/QString>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QDebug>

#include "ewsclient.h"

static const QString distinguishedIdNames[] = {
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

EwsId::EwsId(QXmlStreamReader &reader)
    : mDid(EwsDIdCalendar)
{
    // Don't check for this element's name as a folder id may be contained in several elements
    // such as "FolderId" or "ParentFolderId".
    const QXmlStreamAttributes &attrs = reader.attributes();

    QStringRef idRef = attrs.value(QStringLiteral("Id"));
    QStringRef changeKeyRef = attrs.value(QStringLiteral("ChangeKey"));
    if (idRef.isNull())
        return;

    mId = idRef.toString();
    if (!changeKeyRef.isNull())
        mChangeKey = changeKeyRef.toString();

    mType = Real;
}

EwsId& EwsId::operator=(const EwsId &other)
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

EwsId& EwsId::operator=(EwsId &&other)
{
    mType = other.mType;
    if (mType == Distinguished) {
        mDid = other.mDid;
    }
    else if (mType == Real) {
        mId = std::move(other.mId);
        mChangeKey = std::move(other.mChangeKey);
    }
    return *this;
}

bool EwsId::operator==(const EwsId &other) const
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

void EwsId::writeFolderIds(QXmlStreamWriter &writer) const
{
    if (mType == Distinguished) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("DistinguishedFolderId"));
        writer.writeAttribute(QStringLiteral("Id"), distinguishedIdNames[mDid]);
        writer.writeEndElement();
    }
    else if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("FolderId"));
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
        }
        writer.writeEndElement();
    }
}

void EwsId::writeItemIds(QXmlStreamWriter &writer) const
{
    if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("ItemId"));
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (!mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mChangeKey);
        }
        writer.writeEndElement();
    }
}

QDebug operator<<(QDebug debug, const EwsId &id)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsId(");

    switch (id.mType)
    {
    case EwsId::Distinguished:
        d << QStringLiteral("Distinguished: ") << distinguishedIdNames[id.mDid];
        break;
    case EwsId::Real:
    {
        QString name = EwsClient::folderHash.value(id.mId);
        if (name.isNull()) {
            name = id.mId;
        }
        d << name << QStringLiteral(", changeKey: ") << id.mChangeKey;
        break;
    }
    default:
        break;
    }
    d << ')';
    return debug;
}

uint qHash(const EwsId &id, uint seed)
{
    return qHash(id.id(), seed) ^ qHash(id.changeKey(), seed) ^ static_cast<uint>(id.type());
}
