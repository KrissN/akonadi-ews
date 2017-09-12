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

#ifndef EWSCREATEITEMJOB_H
#define EWSCREATEITEMJOB_H

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>
#include "ewsjob.h"

class EwsClient;
class EwsItem;
class EwsTagStore;
class EwsResource;

class EwsCreateItemJob : public EwsJob
{
    Q_OBJECT
public:
    EwsCreateItemJob(EwsClient &client, const Akonadi::Item &item,
                     const Akonadi::Collection &collection, EwsTagStore *tagStore, EwsResource *parent);
    virtual ~EwsCreateItemJob();

    virtual bool setSend(bool send = true) = 0;

    const Akonadi::Item &item() const;

    void start() override;
private Q_SLOTS:
    void tagSyncFinished(KJob *job);
protected:
    void populateCommonProperties(EwsItem &item);
    virtual void doStart() = 0;

    Akonadi::Item mItem;
    Akonadi::Collection mCollection;
    EwsClient &mClient;
    EwsTagStore *mTagStore;
};

#endif
