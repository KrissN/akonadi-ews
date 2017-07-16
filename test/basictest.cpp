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
#include "isolatedtestbase.h"

class BasicTest : public IsolatedTestBase
{
    Q_OBJECT
public:
    BasicTest(QObject *parent = 0);
    virtual ~BasicTest();
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testBasic();
};

QTEST_AKONADIMAIN(BasicTest)

using namespace Akonadi;

BasicTest::BasicTest(QObject *parent)
    : IsolatedTestBase(parent)
{
}

BasicTest::~BasicTest()
{
}

void BasicTest::initTestCase()
{
    init();
}

void BasicTest::cleanupTestCase()
{
    cleanup();
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

    mFakeServerThread->setDialog(dialog);

    mEwsInstance->setIsOnline(true);

    QThread::sleep(15);

    mEwsInstance->setIsOnline(false);

    QThread::sleep(1);
}

#include "basictest.moc"
