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

#ifndef EWSGLOBALTAGSREADJOB_H
#define EWSGLOBALTAGSREADJOB_H

#include "ewsjob.h"

#include <AkonadiCore/Tag>

class EwsTagStore;
class EwsClient;
namespace Akonadi {
class Collection;
}

class EwsGlobalTagsReadJob : public EwsJob
{
    Q_OBJECT
public:
    EwsGlobalTagsReadJob(EwsTagStore *tagStore, EwsClient &client,
                         const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsGlobalTagsReadJob();

    void start() Q_DECL_OVERRIDE;

    const Akonadi::Tag::List &tags() const { return mTags; };
private Q_SLOTS:
    void getFolderRequestFinished(KJob *job);
private:
    EwsTagStore *mTagStore;
    EwsClient &mClient;
    const Akonadi::Collection &mRootCollection;
    Akonadi::Tag::List mTags;
};

#endif
