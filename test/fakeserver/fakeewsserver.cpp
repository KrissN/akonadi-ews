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

static const QHash<uint, QLatin1String> responseCodes = {
    {200, QLatin1String("OK")},
    {400, QLatin1String("Bad Request")},
    {401, QLatin1String("Unauthorized")},
    {403, QLatin1String("Forbidden")},
    {404, QLatin1String("Not Found")},
    {405, QLatin1String("Method Not Allowed")},
    {500, QLatin1String("Internal Server Error")}
};

FakeEwsServer::FakeEwsServer(const DialogEntry::List &dialog,
                             DialogEntry::ReplyCallback defaultReplyCallback, QObject *parent)
    : QTcpServer(parent), mDialog(dialog), mDefaultReplyCallback(defaultReplyCallback)
{
    listen(QHostAddress::LocalHost, Port);

    connect(this, &QTcpServer::newConnection, this, &FakeEwsServer::newConnectionReceived);
}

FakeEwsServer::~FakeEwsServer()
{
}

void FakeEwsServer::newConnectionReceived()
{
    QTcpSocket *sock = nextPendingConnection();

    auto replyCallback = std::bind(&FakeEwsServer::parseRequest, this, std::placeholders::_1);
    new FakeEwsConnection(sock, replyCallback, this);

    //connect(sock, &QObject::destroyed, this, &FakeEwsServer::connectionClosed);*/
    //connect(mConnection, &FakeEwsConnection::requestReceived, this, &FakeEwsServer::requestReceived);*/
/*    connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
        FakeEwsServer::dataAvailable(sock);
    });*/
}

FakeEwsServer::DialogEntry::HttpResponse FakeEwsServer::parseRequest(const QString &content)
{
    //qDebug() << "Got request" << content;
    Q_FOREACH(const DialogEntry &de, mDialog) {
        if ((!de.expected.isNull() && content == de.expected) ||
            (de.expectedRe.isValid() && de.expectedRe.match(content).hasMatch())) {
            //qDebug() << "Matched entry";
            if (de.conditionCallback && !de.conditionCallback(content)) {
                //qDebug() << "Condition callback returned false - skipping";
                continue;
            }
            if (de.replyCallback) {
                //qDebug() << "Found reply callback - calling";
                return de.replyCallback(content);
            }
            //qDebug() << "Returning static response";
            return de.response;
        }
    }

    if (mDefaultReplyCallback) {
        return mDefaultReplyCallback(content);
    } else {
        return { QStringLiteral(""), 500 };
    }
}

/*void FakeEwsServer::connectionClosed(QObject *obj)
{
    if (mConnection == qobject_cast<FakeEwsConnection*>(obj)) {
        mConnection = Q_NULLPTR;
    }
}

void FakeEwsServer::sendResponse(const QByteArray &resp, bool closeConn)
{
    if (mConnection) {
        mConnection->sendResponse(resp, closeConn);
    }
    else {
        qWarning() << QStringLiteral("No server connection active.");
    }
}

void FakeEwsServer::sendError(const QLatin1String &msg, ushort code)
{
    if (mConnection) {
        mConnection->sendError(msg, code);
    }
    else {
        qWarning() << QStringLiteral("No server connection active.");
    }
}*/
