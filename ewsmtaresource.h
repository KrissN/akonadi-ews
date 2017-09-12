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

#ifndef EWSMTARESOURCE_H
#define EWSMTARESOURCE_H

#include <akonadi_version.h>
#include <AkonadiAgentBase/ResourceBase>
#include <AkonadiAgentBase/TransportResourceBase>
#include <AkonadiCore/Item>

// Some older variants of akonadi_version.h use a different name
#ifndef AKONADI_VERSION
#define AKONADI_VERSION AKONADILIBRARIES_VERSION
#endif

class OrgKdeAkonadiEwsResourceInterface;

class EwsMtaResource : public Akonadi::ResourceBase, public Akonadi::TransportResourceBase
{
    Q_OBJECT
public:
    explicit EwsMtaResource(const QString &id);
    ~EwsMtaResource();

    virtual void sendItem(const Akonadi::Item &item) override;
public Q_SLOTS:
    void configure(WId windowId) override;
protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
#if (AKONADI_VERSION > 0x50328)
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
#else
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
#endif
private Q_SLOTS:
    void messageSent(QString id, QString error);
private:
    bool connectEws();

    OrgKdeAkonadiEwsResourceInterface *mEwsResource;
    QHash<QString, Akonadi::Item> mItemHash;
};

#endif
