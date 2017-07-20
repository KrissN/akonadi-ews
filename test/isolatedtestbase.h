/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef TEST_ISOLATEDTESTBASE_H_
#define TEST_ISOLATEDTESTBASE_H_

#include "fakeewsserver.h"
#include <Akonadi/KMime/SpecialMailCollections>
#include <QObject>
#include <QString>

namespace Akonadi {
class AgentInstance;
}
class FakeEwsServerThread;
class OrgKdeAkonadiEwsSettingsInterface;
class OrgKdeAkonadiEwsWalletInterface;

class IsolatedTestBase : public QObject
{
    Q_OBJECT
public:
    class Folder {
    public:
        enum DistinguishedType {
            None,
            Root,
            Inbox,
            Outbox,
            Sent,
            Trash,
            Drafts,
            Templates,
            Calendar,
            Tasks,
            Contacts
        };

        QString id;
        QString name;
        DistinguishedType type;
        QString parentId;
    };

    typedef QVector<Folder> FolderList;

    IsolatedTestBase(QObject *parent = 0);
    virtual ~IsolatedTestBase();

    static QString loadResourceAsString(const QString &path);
protected:
    virtual void init();
    virtual void cleanup();

    bool setEwsResOnline(bool online, bool wait);
protected:
    QString mEwsResIdentifier;
    QString mAkonadiInstanceIdentifier;
    QScopedPointer<FakeEwsServerThread> mFakeServerThread;
    QScopedPointer<Akonadi::AgentInstance> mEwsInstance;
    QScopedPointer<OrgKdeAkonadiEwsSettingsInterface> mEwsSettingsInterface;
    QScopedPointer<OrgKdeAkonadiEwsWalletInterface> mEwsWalletInterface;
};

class DialogEntryBase : public FakeEwsServer::DialogEntry
{
public:
    DialogEntryBase(const QString &descr = QString(), const ReplyCallback &callback = ReplyCallback())
    {
        replyCallback = callback;
        description = descr;
    };
};

class MsgRootInboxDialogEntry : public DialogEntryBase
{
public:
    MsgRootInboxDialogEntry(const QString &rootId, const QString &inboxId,
                            const QString &descr = QString(),
                            const ReplyCallback &callback = ReplyCallback());
};

class SubscribedFoldersDialogEntry : public DialogEntryBase
{
public:
    SubscribedFoldersDialogEntry(const IsolatedTestBase::FolderList &folders,
                                 const QString &descr = QString(),
                                 const ReplyCallback &callback = ReplyCallback());
};

class GetTagsEmptyDialogEntry : public DialogEntryBase
{
public:
    GetTagsEmptyDialogEntry(const QString &rootId, const QString &descr = QString(),
                            const ReplyCallback &callback = ReplyCallback());
};

class SubscribeStreamingDialogEntry : public DialogEntryBase
{
public:
    SubscribeStreamingDialogEntry(const QString &descr = QString(),
                                  const ReplyCallback &callback = ReplyCallback());
};

class SyncFolderHierInitialDialogEntry : public DialogEntryBase
{
public:
    SyncFolderHierInitialDialogEntry(const IsolatedTestBase::FolderList &folders,
                                     const QString &syncState,
                                     const QString &descr = QString(),
                                     const ReplyCallback &callback = ReplyCallback());
};

class UnsubscribeDialogEntry : public DialogEntryBase
{
public:
    UnsubscribeDialogEntry(const QString &descr = QString(),
                           const ReplyCallback &callback = ReplyCallback());
};

#endif
