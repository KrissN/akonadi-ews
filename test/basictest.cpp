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
#include <qtest_akonadi.h>

#include "fakeewsserverthread.h"
#include "ewssettings.h"
#include "ewswallet.h"

class BasicTest : public QObject
{
    Q_OBJECT
public:
    BasicTest(QObject *parent = 0);
    virtual ~BasicTest();
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testBasic();
public:
    QString mEwsResIdentifier;
    QString mAkonadiInstanceIdentifier;
    FakeEwsServerThread mFakeServerThread;
    Akonadi::AgentInstance mEwsInstance;
    QScopedPointer<OrgKdeAkonadiEwsSettingsInterface> mEwsSettingsInterface;
    QScopedPointer<OrgKdeAkonadiEwsWalletInterface> mEwsWalletInterface;
};

QTEST_AKONADIMAIN(BasicTest)

using namespace Akonadi;

BasicTest::BasicTest(QObject *parent)
    : QObject(parent)
{
    qsrand(QDateTime::currentDateTime().toTime_t());
}

BasicTest::~BasicTest()
{
}

void BasicTest::initTestCase()
{
    QVERIFY(Control::start());

    /* Switch all resources offline to reduce interference from them */
    foreach (AgentInstance agent, AgentManager::self()->instances()) {
        agent.setIsOnline(false);
    }

    connect(AgentManager::self(), &AgentManager::instanceAdded, this,
            [](const Akonadi::AgentInstance &instance) {
        qDebug() << "AgentInstanceAdded" << instance.identifier();
    });

    AgentType ewsType = AgentManager::self()->type(QStringLiteral("akonadi_ews_resource"));
    AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob(ewsType);
    QVERIFY(agentCreateJob->exec());
    mEwsInstance = agentCreateJob->instance();
    mEwsResIdentifier = mEwsInstance.identifier();
    mAkonadiInstanceIdentifier = QProcessEnvironment::systemEnvironment().value(QStringLiteral("AKONADI_INSTANCE"));

    mFakeServerThread.start();
    QVERIFY(mFakeServerThread.waitServerStarted());
    qDebug() << mFakeServerThread.portNumber();

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
    QString ewsUrl = QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(mFakeServerThread.portNumber());
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

    qDebug() << "dsko";
}

void BasicTest::cleanupTestCase()
{
    mFakeServerThread.exit();
    mFakeServerThread.wait();
}

static QString loadResourceAsString(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        return QString(f.readAll());
    }
    return QString();
}

void BasicTest::testBasic()
{
    FakeEwsServer::DialogEntry::List dialog = {
        {
            loadResourceAsString(":/xquery/getfolder-inbox-msgroot"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("GetFolder request for inbox and msgroot")
        },
        {
            loadResourceAsString(":/xquery/getfolder-specialfolders"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("GetFolder request for special folders")
        },
        {
            loadResourceAsString(":/xquery/getfolder-tags"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("GetFolder request for tags")
        },
        {
            loadResourceAsString(":/xquery/subscribe-streaming"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Subscribe request for streaming events")
        },
        {
            loadResourceAsString(":/xquery/syncfolderhierarhy-emptystate"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("SyncFolderHierarchy request with empty state")
        },
        {
            loadResourceAsString(":/xquery/unsubscribe"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Unsubscribe request")
        }

    };

    mFakeServerThread.setDialog(dialog);

    mEwsInstance.setIsOnline(true);

    QThread::sleep(15);

    mEwsInstance.setIsOnline(false);

    QThread::sleep(1);
}

#include "basictest.moc"
