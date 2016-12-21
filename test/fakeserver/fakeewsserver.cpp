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

void FakeEwsServer::newConnectionReceived()
{
    QTcpSocket *sock = nextPendingConnection();

    new FakeEwsConnection(sock, this);
}

FakeEwsServer::DialogEntry::HttpResponse FakeEwsServer::parseRequest(const QString &content)
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got request: ") << content;

    Q_FOREACH(const DialogEntry &de, mDialog) {
        if ((!de.expected.isNull() && content == de.expected) ||
            (de.expectedRe.isValid() && de.expectedRe.match(content).hasMatch())) {
            if (EWSFAKE_LOG().isDebugEnabled()) {
                if (!de.expected.isNull() && content == de.expected) {
                    qCDebugNC(EWSFAKE_LOG) << QStringLiteral("Matched entry \"") << de.description
                            << QStringLiteral("\" by expected string");
                } else {
                    qCDebugNC(EWSFAKE_LOG) << QStringLiteral("Matched entry \"") << de.description
                            << QStringLiteral("\" by regular expression");
                }
            }
            if (de.conditionCallback && !de.conditionCallback(content)) {
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Condition callback for entry \"")
                        << de.description << QStringLiteral("\" returned false - skipping");
                continue;
            }
            if (de.replyCallback) {
                auto resp = de.replyCallback(content);
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning response from callback ")
                        << resp.second << QStringLiteral(": ") << resp.first;
                return resp;
            }
            qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning static response ")
                    << de.response.second << QStringLiteral(": ") << de.response.first;
            return de.response;
        }
    }

    if (mDefaultReplyCallback) {
        auto resp = mDefaultReplyCallback(content);
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning response from default callback ")
                << resp.second << QStringLiteral(": ") << resp.first;
        return resp;
    } else {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning default response 500.");
        return { QStringLiteral(""), 500 };
    }
}
