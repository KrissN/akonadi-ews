/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef EWSITEMHANDLER_H
#define EWSITEMHANDLER_H

#include <functional>

#include <QSharedPointer>

#include "ewspropertyfield.h"
#include "ewstypes.h"

namespace Akonadi
{
class Collection;
class Item;
}
class EwsClient;
class EwsFetchItemDetailJob;
class EwsModifyItemJob;
class EwsCreateItemJob;
class EwsItem;
class EwsTagStore;
class EwsResource;

class EwsItemHandler
{
public:
    virtual ~EwsItemHandler();

    virtual EwsFetchItemDetailJob *fetchItemDetailJob(EwsClient &client, QObject *parent,
            const Akonadi::Collection &collection) = 0;
    virtual void setSeenFlag(Akonadi::Item &item, bool value) = 0;
    virtual QString mimeType() = 0;
    virtual bool setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem) = 0;
    virtual EwsModifyItemJob *modifyItemJob(EwsClient &client, const QVector<Akonadi::Item> &items,
                                            const QSet<QByteArray> &parts, QObject *parent) = 0;
    virtual EwsCreateItemJob *createItemJob(EwsClient &client, const Akonadi::Item &item,
                                            const Akonadi::Collection &collection,
                                            EwsTagStore *tagStore, EwsResource *parent) = 0;

    typedef std::function<EwsItemHandler*()> ItemHandlerFactory;
    static void registerItemHandler(EwsItemType type, ItemHandlerFactory factory);
    static EwsItemHandler *itemHandler(EwsItemType type);
    static EwsItemType mimeToItemType(const QString &mimeType);
    static QHash<EwsPropertyField, QVariant> writeFlags(const QSet<QByteArray> &flags);
    static QSet<QByteArray> readFlags(const EwsItem &item);
    static QList<EwsPropertyField> flagsProperties();
    static QList<EwsPropertyField> tagsProperties();
private:
    struct HandlerFactory {
        EwsItemType type;
        ItemHandlerFactory factory;
    };
};

#define EWS_DECLARE_ITEM_HANDLER(clsname, type) \
    class type ## _itemhandler_registrar { \
    public: \
        type ## _itemhandler_registrar()  \
        { \
            EwsItemHandler::registerItemHandler(type, &clsname::factory); \
        } \
    }; \
    const type ## _itemhandler_registrar type ## _itemhandler_registrar_object;

#endif
