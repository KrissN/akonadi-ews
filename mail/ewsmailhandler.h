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

#ifndef EWSMAILHANDLER_H
#define EWSMAILHANDLER_H

#include "ewsitemhandler.h"

class EwsMailHandler : public EwsItemHandler
{
public:
    EwsMailHandler();
    ~EwsMailHandler() override;

    EwsFetchItemDetailJob *fetchItemDetailJob(EwsClient &client, QObject *parent,
                                              const Akonadi::Collection &collection) override;
    void setSeenFlag(Akonadi::Item &item, bool value) override;
    QString mimeType() override;
    bool setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem) override;
    EwsModifyItemJob *modifyItemJob(EwsClient &client, const QVector<Akonadi::Item> &items,
                                    const QSet<QByteArray> &parts, QObject *parent) override;
    EwsCreateItemJob *createItemJob(EwsClient &client, const Akonadi::Item &item,
                                    const Akonadi::Collection &collection,
                                    EwsTagStore *tagStore, EwsResource *parent) override;
    static EwsItemHandler *factory();
    static QHash<EwsPropertyField, QVariant> writeFlags(const QSet<QByteArray> flags);
    static QSet<QByteArray> readFlags(const EwsItem &item);
    static QList<EwsPropertyField> flagsProperties();
};

#endif
