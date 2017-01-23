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

#include "ewsmtaresource.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <KI18n/KLocalizedString>
#include <KMime/Message>

#include "mtaconfigdialog.h"
#include "mtasettings.h"
#include "ewsclient_debug.h"

#include "resourceinterface.h"

using namespace Akonadi;

EwsMtaResource::EwsMtaResource(const QString &id)
    : Akonadi::ResourceBase(id), mEwsResource(Q_NULLPTR)
{
    qDebug() << "EwsMtaResource";
}

EwsMtaResource::~EwsMtaResource()
{
}

bool EwsMtaResource::connectEws()
{
    if (mEwsResource) {
        return true;
    }
    mEwsResource = new OrgKdeAkonadiEwsResourceInterface(QStringLiteral("org.freedesktop.Akonadi.Resource.") + MtaSettings::ewsResource(),
                                                         QStringLiteral("/"), QDBusConnection::sessionBus(), this);
    qDebug() << QStringLiteral("org.freedesktop.akonadi.Resource.") + MtaSettings::ewsResource() << mEwsResource->isValid();
    if (!mEwsResource->isValid()) {
        delete mEwsResource;
        mEwsResource = Q_NULLPTR;
    } else {
        connect(mEwsResource, &OrgKdeAkonadiEwsResourceInterface::messageSent, this, &EwsMtaResource::messageSent);
    }

    return mEwsResource != Q_NULLPTR;
}

void EwsMtaResource::configure(WId windowId)
{
    MtaConfigDialog dlg(this, windowId);
    if (dlg.exec()) {
        MtaSettings::self()->save();
    }
}

void EwsMtaResource::sendItem(const Akonadi::Item &item)
{
    qDebug() << "sendItem" << item.remoteId();

    if (!connectEws()) {
        itemSent(item, TransportFailed, i18n("Unable to connect to master EWS resource"));
        return;
    }

    mItemHash.insert(item.remoteId(), item);

    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
    /* Exchange doesn't just store whatever MIME content that was sent to it - it will parse it and send
     * further the version assembled back from the parsed parts. It seems that this parsing doesn't work well
     * with the quoted-printable encoding, which KMail prefers. This results in malformed encoding, which the
     * sender doesn't even see.
     * As a workaround force encoding of the body (or in case of multipart - all parts) to Base64. */
    if (msg->contents().isEmpty()) {
        msg->changeEncoding(KMime::Headers::CEbase64);
        msg->contentTransferEncoding(true)->setEncoding(KMime::Headers::CEbase64);
    } else {
        Q_FOREACH (KMime::Content *content, msg->contents()) {
            content->changeEncoding(KMime::Headers::CEbase64);
            content->contentTransferEncoding(true)->setEncoding(KMime::Headers::CEbase64);
        }
    }
    msg->assemble();
    QByteArray mimeContent = msg->encodedContent(true);
    mEwsResource->sendMessage(item.remoteId(), mimeContent);
}

void EwsMtaResource::messageSent(QString id, QString error)
{
    qDebug() << "messageSent" << id << error;
    QHash<QString, Item>::iterator it = mItemHash.find(id);
    if (it != mItemHash.end()) {
        if (error.isEmpty()) {
            itemSent(*it, TransportSucceeded, QString());
        } else {
            itemSent(*it, TransportFailed, error);
        }
        mItemHash.erase(it);
    }
}

void EwsMtaResource::retrieveCollections()
{
    collectionsRetrieved(Collection::List());
}

void EwsMtaResource::retrieveItems(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection)

    itemsRetrieved(Item::List());
}

#if (AKONADI_VERSION > 0x50328)
bool EwsMtaResource::retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    itemsRetrieved(items);

    return true;
}
#else
bool EwsMtaResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    itemRetrieved(item);

    return true;
}
#endif

AKONADI_RESOURCE_MAIN(EwsMtaResource)
