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

