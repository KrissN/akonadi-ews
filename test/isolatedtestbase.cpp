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

#include <AkonadiCore/AgentInstanceCreateJob>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/Control>
#include <QtTest/QTest>

#include "isolatedtestbase.h"
#include "fakeewsserverthread.h"
#include "ewssettings.h"
#include "ewswallet.h"

using namespace Akonadi;

IsolatedTestBase::IsolatedTestBase(QObject *parent)
    : QObject(parent), mFakeServerThread(new FakeEwsServerThread(this))
{
    qsrand(QDateTime::currentDateTime().toTime_t());
}

IsolatedTestBase::~IsolatedTestBase()
{
}

void IsolatedTestBase::init()
{
    QVERIFY(Control::start());

    /* Switch all resources offline to reduce interference from them */
    for (AgentInstance agent : AgentManager::self()->instances()) {
        agent.setIsOnline(false);
    }

    connect(AgentManager::self(), &AgentManager::instanceAdded, this,
            [](const Akonadi::AgentInstance &instance) {
        qDebug() << "AgentInstanceAdded" << instance.identifier();
    });

    AgentType ewsType = AgentManager::self()->type(QStringLiteral("akonadi_ews_resource"));
    AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob(ewsType);
    QVERIFY(agentCreateJob->exec());
    mEwsInstance.reset(new AgentInstance(agentCreateJob->instance()));
    mEwsResIdentifier = mEwsInstance->identifier();
    mAkonadiInstanceIdentifier = QProcessEnvironment::systemEnvironment().value(QStringLiteral("AKONADI_INSTANCE"));

    mFakeServerThread->start();
    QVERIFY(mFakeServerThread->waitServerStarted());
    qDebug() << mFakeServerThread->portNumber();

    mEwsSettingsInterface.reset(new OrgKdeAkonadiEwsSettingsInterface(
        QStringLiteral("org.freedesktop.Akonadi.Resource.") + mEwsResIdentifier
            + QStringLiteral(".") + mAkonadiInstanceIdentifier,
        QStringLiteral("/Settings"), QDBusConnection::sessionBus(), this));
    QVERIFY(mEwsSettingsInterface->isValid());

    mEwsWalletInterface.reset(new OrgKdeAkonadiEwsWalletInterface(
        QStringLiteral("org.freedesktop.Akonadi.Resource.") + mEwsResIdentifier
            + QStringLiteral(".") + mAkonadiInstanceIdentifier,
        QStringLiteral("/Settings"), QDBusConnection::sessionBus(), this));
    QVERIFY(mEwsWalletInterface->isValid());

    /* The EWS resource initializes its DBus adapters asynchronously. Therefore it can happen that
     * due to a race access is attempted prior to their initialization. To fix this retry the DBus
     * communication a few times before declaring failure. */
    QDBusReply<QString> reply;
    int retryCnt = 4;
    QString ewsUrl = QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(mFakeServerThread->portNumber());
    do {
        mEwsSettingsInterface->setBaseUrl(ewsUrl);
        reply = mEwsSettingsInterface->baseUrl();
        if (!reply.isValid()) {
            qDebug() << "Failed to set base URL:" << reply.error().message();
            QThread::usleep(250);
        }
    } while (!reply.isValid() && retryCnt-- > 0);
    QVERIFY(reply.isValid());
    QVERIFY(reply.value() == ewsUrl);

    mEwsSettingsInterface->setUsername(QStringLiteral("test"));
    reply = mEwsSettingsInterface->username();
    QVERIFY(reply.isValid());
    QVERIFY(reply.value() == QStringLiteral("test"));

    mEwsWalletInterface->setTestPassword(QStringLiteral("test"));
    AgentManager::self()->instance(mEwsResIdentifier).reconfigure();
}

void IsolatedTestBase::cleanup()
{
    mFakeServerThread->exit();
    mFakeServerThread->wait();
}

QString IsolatedTestBase::loadResourceAsString(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        return QString(f.readAll());
    }
    return QString();
}

bool IsolatedTestBase::setEwsResOnline(bool online, bool wait)
{
    if (wait) {
        QEventLoop loop;
        auto conn = connect(AgentManager::self(), &AgentManager::instanceOnline, this,
                [&](const AgentInstance &instance, bool state) {
            if (instance == *mEwsInstance && state == online) {
                qDebug() << "is online" << state;
                loop.exit(0);
            }
        }, Qt::QueuedConnection);
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, this, [&]() {
            qDebug() << "Timeout waiting for desired resource online state.";
            loop.exit(1);
        });
        timer.start(3000);
        mEwsInstance->setIsOnline(online);
        bool status = (loop.exec() == 0);
        disconnect(conn);
        return status;
    } else {
        mEwsInstance->setIsOnline(online);
        return true;
    }
}


MsgRootInboxDialogEntry::MsgRootInboxDialogEntry(const QString &rootId, const QString &inboxId,
                                                 const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/getfolder-inbox-msgroot").arg(rootId).arg(inboxId);
    description = QStringLiteral("GetFolder request for inbox and msgroot");
}

SubscribedFoldersDialogEntry::SubscribedFoldersDialogEntry(const IsolatedTestBase::FolderList &list,
                                                           const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    static const QVector<IsolatedTestBase::Folder::DistinguishedType> specialFolders = {
        IsolatedTestBase::Folder::Inbox,
        IsolatedTestBase::Folder::Calendar,
        IsolatedTestBase::Folder::Tasks,
        IsolatedTestBase::Folder::Contacts
    };
    QHash<IsolatedTestBase::Folder::DistinguishedType, const IsolatedTestBase::Folder*> folderHash;
    for (const auto &folder : list) {
        if (specialFolders.contains(folder.type)) {
            folderHash.insert(folder.type, &folder);
        }
    }

    QString xml;
    for (auto special : specialFolders) {
        const IsolatedTestBase::Folder *folder = folderHash[special];
        if (QTest::qVerify(folder != nullptr, "folder != nullptr", "", __FILE__, __LINE__)) {
            xml += QStringLiteral("<m:GetFolderResponseMessage ResponseClass=\"Success\">");
            xml += QStringLiteral("<m:ResponseCode>NoError</m:ResponseCode>");
            xml += QStringLiteral("<m:Folders><t:Folder>");
            xml += QStringLiteral("<t:FolderId Id=\"%1\" ChangeKey=\"MDAx\" />").arg(folder->id);
            xml += QStringLiteral("</t:Folder></m:Folders>");
            xml += QStringLiteral("</m:GetFolderResponseMessage>");
        }
    }

    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/getfolder-subscribedfolders").arg(xml);
}

GetTagsEmptyDialogEntry::GetTagsEmptyDialogEntry(const QString &rootId, const QString &descr,
                                                 const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/getfolder-tags").arg(rootId);
}

SubscribeStreamingDialogEntry::SubscribeStreamingDialogEntry(const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/subscribe-streaming");
}

SyncFolderHierInitialDialogEntry::SyncFolderHierInitialDialogEntry(const IsolatedTestBase::FolderList &list,
                                                                   const QString &syncState,
                                                                   const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    QHash<QString, int> childCount;
    for (const auto &folder : list) {
        ++childCount[folder.parentId];
    }

    QString xml;
    for (const auto &folder : list) {
        if (folder.type == IsolatedTestBase::Folder::Root) {
            continue;
        }
        xml += QStringLiteral("<t:Create>");
        xml += QStringLiteral("<t:Folder>");
        xml += QStringLiteral("<t:FolderId Id=\"%1\" ChangeKey=\"MDAx\" />").arg(folder.id);
        xml += QStringLiteral("<t:ParentFolderId Id=\"%1\" ChangeKey=\"MDAx\" />").arg(folder.parentId);
        QString folderClass;
        QString extraXml;
        switch (folder.type) {
            case IsolatedTestBase::Folder::Calendar:
                folderClass = QStringLiteral("IPF.Calendar");
                break;
            case IsolatedTestBase::Folder::Contacts:
                folderClass = QStringLiteral("IPF.Contacts");
                break;
            case IsolatedTestBase::Folder::Tasks:
                folderClass = QStringLiteral("IPF.Tasks");
                break;
            default:
                folderClass = QStringLiteral("IPF.Note");
                extraXml = QStringLiteral("<t:UnreadCount>0</t:UnreadCount>");
        }
        xml += QStringLiteral("<t:FolderClass>%1</t:FolderClass>").arg(folderClass);
        xml += QStringLiteral("<t:TotalCount>0</t:TotalCount>");
        xml += QStringLiteral("<t:DisplayName>%1</t:DisplayName>").arg(folder.name);
        xml += QStringLiteral("<t:ChildFolderCount>%1</t:ChildFolderCount>").arg(childCount[folder.id]);
        xml += extraXml;
        xml += QStringLiteral("</t:Folder>");
        xml += QStringLiteral("</t:Create>");

    }
    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/syncfolderhierarhy-emptystate").arg(syncState).arg(xml);
}

UnsubscribeDialogEntry::UnsubscribeDialogEntry(const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    xQuery = IsolatedTestBase::loadResourceAsString(":/xquery/unsubscribe");
}

