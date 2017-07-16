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

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntryMsgRootInbox()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/getfolder-inbox-msgroot"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("GetFolder request for inbox and msgroot")
    });
}

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntrySpecialFolders()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/getfolder-specialfolders"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("GetFolder request for special folders")
    });
}

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntryGetTagsEmpty()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/getfolder-tags"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("GetFolder request for tags")
    });
}

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntrySubscribeStreaming()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/subscribe-streaming"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("Subscribe request for streaming events")
    });
}

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntrySyncFolderHierarchyEmptyState()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/syncfolderhierarhy-emptystate"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("SyncFolderHierarchy request with empty state")
    });
}

FakeEwsServer::DialogEntry IsolatedTestBase::dialogEntryUnsubscribe()
{
    return FakeEwsServer::DialogEntry({
        loadResourceAsString(":/xquery/unsubscribe"),
        FakeEwsServer::DialogEntry::ReplyCallback(),
        QStringLiteral("Unsubscribe request")
    });
}

