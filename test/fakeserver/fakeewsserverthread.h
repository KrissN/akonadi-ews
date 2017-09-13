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

#ifndef FAKEEWSSERVERTHREAD_H
#define FAKEEWSSERVERTHREAD_H

#include <QMutex>
#include <QThread>

#include "fakeewsserver.h"

class Q_DECL_EXPORT FakeEwsServerThread : public QThread
{
    Q_OBJECT
public:
    explicit FakeEwsServerThread(QObject *parent = 0);
    virtual ~FakeEwsServerThread();

//    FakeEwsServer *server() const;
    ushort portNumber() const
    {
        return mPortNumber;
    };
    bool isRunning() const
    {
        return mIsRunning == 1;
    };
    void setDialog(const FakeEwsServer::DialogEntry::List &dialog);
    void setDefaultReplyCallback(FakeEwsServer::DialogEntry::ReplyCallback defaultReplyCallback);
    void queueEventsXml(const QStringList &events);
    bool waitServerStarted() const;
Q_SIGNALS:
    void serverStarted(bool ok);
protected:
    virtual void run() override;
private Q_SLOTS:
    void doQueueEventsXml(const QStringList events);
private:
    QScopedPointer<FakeEwsServer> mServer;
    ushort mPortNumber;
    QAtomicInt mIsRunning;
    mutable QMutex mMutex;
};

#endif
