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

#include "ewstaskhandler.h"

#include <KCalCore/Todo>

#include "ewsfetchtaskdetailjob.h"
#include "ewsmodifytaskjob.h"

using namespace Akonadi;

EwsTaskHandler::EwsTaskHandler()
{
}

EwsTaskHandler::~EwsTaskHandler()
{
}

EwsItemHandler *EwsTaskHandler::factory()
{
    return new EwsTaskHandler();
}

EwsFetchItemDetailJob *EwsTaskHandler::fetchItemDetailJob(EwsClient &client, QObject *parent,
                                                          const Akonadi::Collection &collection)
{
    return new EwsFetchTaskDetailJob(client, parent, collection);
}

void EwsTaskHandler::setSeenFlag(Item &item, bool value)
{
    Q_UNUSED(item)
    Q_UNUSED(value)
}

QString EwsTaskHandler::mimeType()
{
    return KCalCore::Todo::todoMimeType();
}

bool EwsTaskHandler::setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem)
{
    Q_UNUSED(item)
    Q_UNUSED(ewsItem)

    return true;
}

EwsModifyItemJob *EwsTaskHandler::modifyItemJob(EwsClient& client, const Akonadi::Item &item,
                                                const QSet<QByteArray> &parts, QObject *parent)
{
    return new EwsModifyTaskJob(client, item, parts, parent);
}


EWS_DECLARE_ITEM_HANDLER(EwsTaskHandler, EwsItemTypeTask)
