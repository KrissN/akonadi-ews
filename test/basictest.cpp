/*
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
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/Control>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/Monitor>
#include <qtest_akonadi.h>

#include "fakeewsserverthread.h"
#include "ewssettings.h"
#include "ewswallet.h"
#include "isolatedtestbase.h"
#include "statemonitor.h"

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
    static const auto rootId = QStringLiteral("cm9vdA==");
    static const auto inboxId = QStringLiteral("aW5ib3g=");
    FolderList folderList = {
        {rootId, mEwsInstance->identifier(), Folder::Root, QString()},
        {inboxId, "Inbox", Folder::Inbox, rootId},
        {"Y2FsZW5kYXI=", "Calendar", Folder::Calendar, rootId},
        {"dGFza3M=", "Tasks", Folder::Tasks, rootId},
        {"Y29udGFjdHM=", "Contacts", Folder::Contacts, rootId},
        {"b3V0Ym94", "Outbox", Folder::Outbox, rootId},
        {"c2VudCBpdGVtcw==", "Sent Items", Folder::Sent, rootId},
        {"ZGVsZXRlZCBpdGVtcw==", "Deleted Items", Folder::Trash, rootId},
        {"ZHJhZnRz", "Drafts", Folder::Drafts, rootId}
    };

    struct DesiredState {
        QString parentId;
        QString iconName;
    };
    QHash<QString, DesiredState> desiredStates = {
        {rootId, {QString(), QString()}},
        {inboxId, {rootId, "mail-folder-inbox"}},
        {"Y2FsZW5kYXI=", {rootId, QString()}},
        {"dGFza3M=", {rootId, QString()}},
        {"Y29udGFjdHM=", {rootId, QString()}},
        {"b3V0Ym94", {rootId, "mail-folder-outbox"}},
        {"c2VudCBpdGVtcw==", {rootId, "mail-folder-sent"}},
        {"ZGVsZXRlZCBpdGVtcw==", {rootId, "user-trash"}},
        {"ZHJhZnRz", {rootId, "document-properties"}}
    };

    FakeEwsServer::DialogEntry::List dialog =
    {
        MsgRootInboxDialogEntry(rootId, inboxId,
                                QStringLiteral("GetFolder request for inbox and msgroot")),
        SubscribedFoldersDialogEntry(folderList,
                                     QStringLiteral("GetFolder request for subscribed folders")),
        SpecialFoldersDialogEntry(folderList,
                                  QStringLiteral("GetFolder request for special folders")),
        GetTagsEmptyDialogEntry(rootId,
                                QStringLiteral("GetFolder request for tags")),
        SubscribeStreamingDialogEntry(QStringLiteral("Subscribe request for streaming events")),
        SyncFolderHierInitialDialogEntry(folderList, "bNUUPDWHTvuG9p57NGZdhjREdZXDt48a0E1F22yThko=",
                                         QStringLiteral("SyncFolderHierarchy request with empty state")),
        UnsubscribeDialogEntry(QStringLiteral("Unsubscribe request"))
    };

    bool unknownRequestEncountered = false;
    mFakeServerThread->setDialog(dialog);
    mFakeServerThread->setDefaultReplyCallback([&](const QString &req, QXmlResultItems &, const QXmlNamePool &) {
        qDebug() << "Unknown EWS request encountered." << req;
        unknownRequestEncountered = true;
        return FakeEwsServer::EmptyResponse;
    });

    QEventLoop loop;

    CollectionStateMonitor<DesiredState> stateMonitor(this, desiredStates, inboxId,
                                                      [](const Collection &col, const DesiredState &state) {
        auto attr = col.attribute<EntityDisplayAttribute>();
        QString iconName;
        if (attr) {
            iconName = attr->iconName();
        }
        return col.parentCollection().remoteId() == state.parentId && iconName == state.iconName;
    });

    stateMonitor.monitor().fetchCollection(true);
    stateMonitor.monitor().collectionFetchScope().fetchAttribute<EntityDisplayAttribute>();
    stateMonitor.monitor().collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
    stateMonitor.monitor().setResourceMonitored(mEwsInstance->identifier().toAscii(), true);
    connect(&stateMonitor, &CollectionStateMonitor<DesiredState>::stateReached, this, [&]() {
        loop.exit(0);
    });
    connect(&stateMonitor, &CollectionStateMonitor<DesiredState>::errorOccurred, this, [&]() {
        loop.exit(1);
    });

    QVERIFY(setEwsResOnline(true, true));

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [&]() {
        qDebug() << "Timeout waiting for desired resource online state.";
        loop.exit(1);
    });
    timer.start(5000);
    QCOMPARE(loop.exec(), 0);

    QVERIFY(!unknownRequestEncountered);
    QVERIFY(setEwsResOnline(false, true));
}

#include "basictest.moc"
