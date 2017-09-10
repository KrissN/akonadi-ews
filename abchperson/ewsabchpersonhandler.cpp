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

#include "ewsabchpersonhandler.h"

#include <KCalCore/Todo>

#include "ewsfetchabchpersondetailjob.h"
#include "ewsmodifyabchpersonjob.h"
#include "ewscreateabchpersonjob.h"

using namespace Akonadi;

EwsAbchPersonHandler::EwsAbchPersonHandler()
{
}

EwsAbchPersonHandler::~EwsAbchPersonHandler()
{
}

EwsItemHandler *EwsAbchPersonHandler::factory()
{
    return new EwsAbchPersonHandler();
}

EwsFetchItemDetailJob *EwsAbchPersonHandler::fetchItemDetailJob(EwsClient &client, QObject *parent,
                                                          const Akonadi::Collection &collection)
{
    return new EwsFetchAbchContactDetailsJob(client, parent, collection);
}

void EwsAbchPersonHandler::setSeenFlag(Item &item, bool value)
{
    Q_UNUSED(item)
    Q_UNUSED(value)
}

QString EwsAbchPersonHandler::mimeType()
{
    return KCalCore::Todo::todoMimeType();
}

bool EwsAbchPersonHandler::setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem)
{
    Q_UNUSED(item)
    Q_UNUSED(ewsItem)

    return true;
}

EwsModifyItemJob *EwsAbchPersonHandler::modifyItemJob(EwsClient& client, const QVector<Akonadi::Item> &items,
                                                const QSet<QByteArray> &parts, QObject *parent)
{
    return new EwsModifyAbchPersonJob(client, items, parts, parent);
}

EwsCreateItemJob *EwsAbchPersonHandler::createItemJob(EwsClient& client, const Akonadi::Item &item,
                                                const Akonadi::Collection &collection,
                                                EwsTagStore *tagStore, EwsResource *parent)
{
    return new EwsCreateAbchPersonJob(client, item, collection, tagStore, parent);
}

EWS_DECLARE_ITEM_HANDLER(EwsAbchPersonHandler, EwsItemTypeAbchPerson)
