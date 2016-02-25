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
    : Akonadi::ResourceBase(id)
{
    qDebug() << "EwsMtaResource";

    mEwsResource = new OrgKdeAkonadiEwsResourceInterface(QStringLiteral("org.freedesktop.Akonadi.Resource.") + MtaSettings::ewsResource(),
                                                         QStringLiteral("/"), QDBusConnection::sessionBus(), this);
    qDebug() << QStringLiteral("org.freedesktop.akonadi.Resource.") + MtaSettings::ewsResource() << mEwsResource->isValid();
}

EwsMtaResource::~EwsMtaResource()
{
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

    mItemHash.insert(item.remoteId(), item);

    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
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

bool EwsMtaResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    itemRetrieved(item);

    return true;
}

AKONADI_RESOURCE_MAIN(EwsMtaResource)
