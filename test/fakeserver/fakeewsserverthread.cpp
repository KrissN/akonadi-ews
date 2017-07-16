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

#include "fakeewsserverthread.h"

#include <QtCore/QEventLoop>
#include "fakeewsserver.h"
#include "fakeewsserver_debug.h"

FakeEwsServerThread::FakeEwsServerThread(QObject *parent)
    : QThread(parent), mPortNumber(0), mIsRunning(0)
{
}

FakeEwsServerThread::~FakeEwsServerThread()
{
}

void FakeEwsServerThread::run()
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Starting fake server thread");
    mMutex.lock();
    mServer.reset(new FakeEwsServer(0));
    bool ok = mServer->start();
    mMutex.unlock();

    if (ok) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Fake server thread started.");
        mPortNumber = mServer->portNumber();
        Q_EMIT serverStarted(ok);
        mIsRunning = 1;
        exec();
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Fake server thread terminating.");
    } else {
        Q_EMIT serverStarted(ok);
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Fake server thread start failed.");
    }

    mMutex.lock();
    mServer.reset();
    mMutex.unlock();
}

void FakeEwsServerThread::setDialog(const FakeEwsServer::DialogEntry::List &dialog)
{
    QMutexLocker lock(&mMutex);

    if (mServer) {
        mServer->setDialog(dialog);
    }
}

void FakeEwsServerThread::setDefaultReplyCallback(FakeEwsServer::DialogEntry::ReplyCallback defaultReplyCallback)
{
    QMutexLocker lock(&mMutex);

    if (mServer) {
        mServer->setDefaultReplyCallback(defaultReplyCallback);
    }
}

void FakeEwsServerThread::queueEventsXml(const QStringList &events)
{
    QMutexLocker lock(&mMutex);

    if (mServer) {
        metaObject()->invokeMethod(this, "doQueueEventsXml", Q_ARG(QStringList, events));
    }
}

void FakeEwsServerThread::doQueueEventsXml(const QStringList events)
{
    mServer->queueEventsXml(events);
}

bool FakeEwsServerThread::waitServerStarted() const
{
    QEventLoop loop;
    {
        QMutexLocker lock(&mMutex);
        if (isFinished()) {
            return false;
        }
        if (mIsRunning) {
            return true;
        }
        connect(this, &FakeEwsServerThread::serverStarted, this, [&loop](bool ok) {
            loop.exit(ok ? 1 : 0);
        });
    }
    return loop.exec();
}
