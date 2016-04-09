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

#include "ewscontacthandler.h"

#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include "ewsfetchcontactdetailjob.h"
#include "ewsmodifycontactjob.h"
#include "ewscreatecontactjob.h"

using namespace Akonadi;

EwsContactHandler::EwsContactHandler()
{
}

EwsContactHandler::~EwsContactHandler()
{
}

EwsItemHandler *EwsContactHandler::factory()
{
    return new EwsContactHandler();
}

EwsFetchItemDetailJob *EwsContactHandler::fetchItemDetailJob(EwsClient &client, QObject *parent,
                                                             const Akonadi::Collection &collection)
{
    return new EwsFetchContactDetailJob(client, parent, collection);
}

void EwsContactHandler::setSeenFlag(Item &item, bool value)
{
    Q_UNUSED(item)
    Q_UNUSED(value)
}

QString EwsContactHandler::mimeType()
{
    return KContacts::Addressee::mimeType();
}

bool EwsContactHandler::setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem)
{
    Q_UNUSED(item)
    Q_UNUSED(ewsItem)

    return true;
}

EwsModifyItemJob *EwsContactHandler::modifyItemJob(EwsClient& client, const QVector<Akonadi::Item> &items,
                                                   const QSet<QByteArray> &parts, QObject *parent)
{
    return new EwsModifyContactJob(client, items, parts, parent);
}

EwsCreateItemJob *EwsContactHandler::createItemJob(EwsClient& client, const Akonadi::Item &item,
                                                   const Akonadi::Collection &collection,
                                                   EwsTagStore *tagStore, EwsResource *parent)
{
    return new EwsCreateContactJob(client, item, collection, tagStore, parent);
}

EWS_DECLARE_ITEM_HANDLER(EwsContactHandler, EwsItemTypeContact)
