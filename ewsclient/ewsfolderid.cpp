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

#include "ewsfolderid.h"

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

EwsFolderId::EwsFolderId(QXmlStreamReader &reader)
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

EwsFolderId& EwsFolderId::operator=(const EwsFolderId &other)
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

EwsFolderId& EwsFolderId::operator=(EwsFolderId &&other)
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

bool EwsFolderId::operator==(const EwsFolderId &other) const
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

void EwsFolderId::writeFolderIds(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderIds"));

    if (mType == Distinguished) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("DistinguishedFolderId"));
        writer.writeAttribute(QStringLiteral("Id"), distinguishedIdNames[mDid]);
        writer.writeEndElement();
    }
    else if (mType == Real) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("FolderId"));
        writer.writeAttribute(QStringLiteral("Id"), mId);
        if (~mChangeKey.isEmpty()) {
            writer.writeAttribute(QStringLiteral("ChangeKey"), mId);
        }
        writer.writeEndElement();
    }

    writer.writeEndElement();
}

