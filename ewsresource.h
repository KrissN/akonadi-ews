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

#ifndef EWSRESOURCE_H
#define EWSRESOURCE_H

#include <AkonadiAgentBase/ResourceBase>

#include "ewsclient.h"

class FetchItemState;
class EwsGetItemRequest;
class EwsFindFolderRequest;
class EwsFolder;

class EwsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT
public:
    explicit EwsResource(const QString &id);
    ~EwsResource();
public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
private Q_SLOTS:
    void findFoldersRequestFinished(EwsFindFolderRequest *req);
    void itemFetchJobFinished(KJob *job);
    void getItemRequestFinished(EwsGetItemRequest *req);
private:
    Akonadi::Collection::List createChildCollections(const EwsFolder &folder, Akonadi::Collection collection);
    Akonadi::Collection createFolderCollection(const EwsFolder &folder);
    void finishItemsFetch(FetchItemState *state);

    EwsClient mEwsClient;
    Akonadi::Collection mRootCollection;
};

#endif
