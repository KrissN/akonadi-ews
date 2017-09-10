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

#ifndef EWSSUBSCRIBEDFOLDERSJOB_H
#define EWSSUBSCRIBEDFOLDERSJOB_H

#include "ewsjob.h"
#include "ewsid.h"

class EwsClient;
class Settings;

class EwsSubscribedFoldersJob : public EwsJob
{
    Q_OBJECT
public:
    EwsSubscribedFoldersJob(EwsClient &client, Settings *settings, QObject *parent);
    ~EwsSubscribedFoldersJob();

    void start() Q_DECL_OVERRIDE;

    EwsId::List folders() { return mFolders; };

    static const EwsId::List &defaultSubscriptionFolders();
private Q_SLOTS:
    void verifySubFoldersRequestFinished(KJob *job);
private:
    EwsId::List mFolders;
    EwsClient &mClient;
    Settings *mSettings;
};

#endif
