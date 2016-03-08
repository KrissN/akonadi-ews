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

#include "ewscalendarhandler.h"

#include <KCalCore/Event>

#include "ewsfetchcalendardetailjob.h"
#include "ewsmodifycalendarjob.h"
#include "ewscreatecalendarjob.h"

using namespace Akonadi;

EwsCalendarHandler::EwsCalendarHandler()
{
}

EwsCalendarHandler::~EwsCalendarHandler()
{
}

EwsItemHandler *EwsCalendarHandler::factory()
{
    return new EwsCalendarHandler();
}

EwsFetchItemDetailJob *EwsCalendarHandler::fetchItemDetailJob(EwsClient &client, QObject *parent,
                                                          const Akonadi::Collection &collection)
{
    return new EwsFetchCalendarDetailJob(client, parent, collection);
}

void EwsCalendarHandler::setSeenFlag(Item &item, bool value)
{
    Q_UNUSED(item)
    Q_UNUSED(value)
}

QString EwsCalendarHandler::mimeType()
{
    return KCalCore::Event::eventMimeType();
}

bool EwsCalendarHandler::setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem)
{
    Q_UNUSED(item)
    Q_UNUSED(ewsItem)

    return true;
}

EwsModifyItemJob *EwsCalendarHandler::modifyItemJob(EwsClient& client, const QVector<Akonadi::Item> &items,
                                                    const QSet<QByteArray> &parts, QObject *parent)
{
    return new EwsModifyCalendarJob(client, items, parts, parent);
}

EwsCreateItemJob *EwsCalendarHandler::createItemJob(EwsClient& client, const Akonadi::Item &item,
                                                    const Akonadi::Collection &collection, QObject *parent)
{
    return new EwsCreateCalendarJob(client, item, collection, parent);
}

EWS_DECLARE_ITEM_HANDLER(EwsCalendarHandler, EwsItemTypeCalendarItem)
