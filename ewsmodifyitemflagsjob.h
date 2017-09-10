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

#ifndef EWSMODIFYITEMFLAGSJOB_H
#define EWSMODIFYITEMFLAGSJOB_H

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

#include "ewstypes.h"
#include "ewsclient.h"
#include "ewsjob.h"

class EwsModifyItemFlagsJob : public EwsJob
{
    Q_OBJECT
public:
    EwsModifyItemFlagsJob(EwsClient &client, QObject *parent, const Akonadi::Item::List,
                          const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags);
    virtual ~EwsModifyItemFlagsJob();

    Akonadi::Item::List items() const
    {
        return mResultItems;
    }

    virtual void start() Q_DECL_OVERRIDE;
protected:
    Akonadi::Item::List mItems;
    Akonadi::Item::List mResultItems;
    EwsClient &mClient;
    QSet<QByteArray> mAddedFlags;
    QSet<QByteArray> mRemovedFlags;
private Q_SLOTS:
    void itemModifyFinished(KJob *job);
private:
};

#endif
