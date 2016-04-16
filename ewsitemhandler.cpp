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

#include "ewsitemhandler.h"

#include "ewsresource.h"
#include "ewsclient_debug.h"

struct HandlerFactory {
    EwsItemType type;
    EwsItemHandler::ItemHandlerFactory factory;
};

typedef QList<HandlerFactory> HandlerList;
typedef QHash<EwsItemType, QSharedPointer<EwsItemHandler>> HandlerHash;

Q_GLOBAL_STATIC(HandlerList, handlerFactories)
Q_GLOBAL_STATIC(HandlerHash, handlers)

EwsItemHandler::~EwsItemHandler()
{
}

void EwsItemHandler::registerItemHandler(EwsItemType type, ItemHandlerFactory factory)
{
    handlerFactories->append({type, factory});
}
EwsItemHandler *EwsItemHandler::itemHandler(EwsItemType type)
{
    HandlerHash::iterator it = handlers->find(type);
    if (it != handlers->end()) {
        return it->data();
    }
    else {
        HandlerList::const_iterator it;
        for (it = handlerFactories->cbegin(); it != handlerFactories->cend(); it++) {
            if (it->type == type) {
                EwsItemHandler *handler = it->factory();
                (*handlers)[type].reset(handler);
                return handler;
            }
        }
        qCWarning(EWSRES_LOG) << QStringLiteral("Could not find handler for item type %1").arg(type);

        return Q_NULLPTR;
    }
}

EwsItemType EwsItemHandler::mimeToItemType(QString mimeType)
{
    if (mimeType == itemHandler(EwsItemTypeMessage)->mimeType()) {
        return EwsItemTypeMessage;
    }
    else if (mimeType == itemHandler(EwsItemTypeCalendarItem)->mimeType()) {
        return EwsItemTypeCalendarItem;
    }
    else if (mimeType == itemHandler(EwsItemTypeTask)->mimeType()) {
        return EwsItemTypeTask;
    }
    else if (mimeType == itemHandler(EwsItemTypeContact)->mimeType()) {
        return EwsItemTypeContact;
    }
    else {
        return EwsItemTypeItem;
    }
}

QHash<EwsPropertyField, QVariant> EwsItemHandler::writeFlags(QSet<QByteArray> flags)
{
    QHash<EwsPropertyField, QVariant> propertyHash;

    if (flags.isEmpty()) {
        propertyHash.insert(EwsResource::flagsProperty, QVariant());
    } else {
        QStringList flagList;
        Q_FOREACH(const QByteArray flag, flags) {
            flagList.append(flag);
        }
        propertyHash.insert(EwsResource::flagsProperty, flagList);
    }

    return propertyHash;
}
