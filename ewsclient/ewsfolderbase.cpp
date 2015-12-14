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

#include "ewsgetfolderrequest.h"
#include "ewsclient_debug.h"

EwsFolderBasePrivate::EwsFolderBasePrivate()
    : mGetFolderReq(0), mValid(false), mUpdated(false), mCurrentShape(EwsShapeDefault),
      mTotalCount(-1), mChildFolderCount(-1)
{
}

EwsFolderBasePrivate::~EwsFolderBasePrivate()
{
    delete mGetFolderReq;
}

EwsFolderBase::EwsFolderBase(EwsFolderBasePrivate *priv, EwsFolderId id, EwsClient *parent)
    : QObject(parent), d(priv)
{
    d->mId = id;
    d->mGetFolderReq = new EwsGetFolderRequest(parent);
    d->mGetFolderReq->setFolderId(d->mId);
    d->mGetFolderReq->setFolderShape(d->mCurrentShape);
}

EwsFolderBase::EwsFolderBase(EwsFolderId id, EwsClient *parent)
    : QObject(parent), d(new EwsFolderBasePrivate())
{
    d->mId = id;
    d->mGetFolderReq = new EwsGetFolderRequest(parent);
    d->mGetFolderReq->setFolderId(d->mId);
    d->mGetFolderReq->setFolderShape(d->mCurrentShape);
}

EwsFolderBase::EwsFolderBase(EwsFolderBasePrivate *priv, EwsClient *parent)
    : QObject(parent), d(priv)
{
}

EwsFolderBase::~EwsFolderBase()
{
}

void EwsFolderBase::requestFinished()
{

}

bool EwsFolderBase::isValid()
{
    return d->mValid;
}

bool EwsFolderBase::readBaseFolderElement(QXmlStreamReader &reader)
{
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
        d->mTotalCount = reader.readElementText().toInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("TotalCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ChildFolderCount")) {
        bool ok;
        d->mChildFolderCount = reader.readElementText().toInt(&ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ChildFolderCount"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ExtendedProperty")) {
        // Unsupported - ignore
    }
    else if (reader.name() == QStringLiteral("ManagedFolderInformation")) {
        // Unsupported - ignore
    }
    else if (reader.name() == QStringLiteral("EffectiveRights")) {
        // Unsupported - ignore
    }
    else {
        qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - unknown element: %1.")
                        .arg(reader.name().toString());
        return false;
    }
    return true;
}

