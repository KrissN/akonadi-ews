/*  This file is part of Akonadi EWS Resource
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

#include "fakeewsserver.h"

#include <QtCore/QCoreApplication>

#include "fakeewsconnection.h"
#include "fakeewsserver_debug.h"

const FakeEwsServer::DialogEntry::HttpResponse FakeEwsServer::EmptyResponse = {QString(), 0};

FakeEwsServer::FakeEwsServer(const DialogEntry::List &dialog, QObject *parent)
    : QTcpServer(parent), mDialog(dialog)
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Starting fake EWS server at 127.0.0.1:") << Port;
    listen(QHostAddress::LocalHost, Port);

    connect(this, &QTcpServer::newConnection, this, &FakeEwsServer::newConnectionReceived);
}

FakeEwsServer::~FakeEwsServer()
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Stopping fake EWS server.");
}

void FakeEwsServer::setDefaultReplyCallback(DialogEntry::ReplyCallback defaultReplyCallback)
{
    mDefaultReplyCallback = defaultReplyCallback;
}

void FakeEwsServer::queueEventsXml(const QStringList &events)
{
    mEventQueue += events;

    if (mStreamingEventsConnection) {
        mStreamingEventsConnection->sendEvents(mEventQueue);
        mEventQueue.clear();
    }
}

QStringList FakeEwsServer::retrieveEventsXml()
{
    QStringList events = mEventQueue;
    mEventQueue.clear();
    return events;
}

void FakeEwsServer::newConnectionReceived()
{
    QTcpSocket *sock = nextPendingConnection();

    FakeEwsConnection *conn = new FakeEwsConnection(sock, this);
    connect(conn, &FakeEwsConnection::streamingRequestStarted, this, &FakeEwsServer::streamingConnectionStarted);
}

const FakeEwsServer::DialogEntry::List FakeEwsServer::dialog() const
{
    return mDialog;
}

const FakeEwsServer::DialogEntry::ReplyCallback FakeEwsServer::defaultReplyCallback() const
{
    return mDefaultReplyCallback;
}

void FakeEwsServer::streamingConnectionStarted(FakeEwsConnection *conn)
{
    if (mStreamingEventsConnection) {
        qCWarningNC(EWSFAKE_LOG) << QStringLiteral("Got new streaming connection while existing one is active - terminating existing one");
        mStreamingEventsConnection->deleteLater();
    }

    mStreamingEventsConnection = conn;
}
