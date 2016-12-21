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

#include "fakeewsconnection.h"

#include <QtNetwork/QTcpSocket>

#include "fakeewsserver_debug.h"

static const QHash<uint, QLatin1String> responseCodes = {
    {200, QLatin1String("OK")},
    {400, QLatin1String("Bad Request")},
    {401, QLatin1String("Unauthorized")},
    {403, QLatin1String("Forbidden")},
    {404, QLatin1String("Not Found")},
    {405, QLatin1String("Method Not Allowed")},
    {500, QLatin1String("Internal Server Error")}
};

FakeEwsConnection::FakeEwsConnection(QTcpSocket *sock,
                                     const FakeEwsServer::DialogEntry::ReplyCallback replyCallback,
                                     QObject* parent)
    : QObject(parent), mSock(sock), mReplyCallback(replyCallback), mContentLength(0), mKeepAlive(false)
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got new EWS connection.");
    connect(mSock.data(), &QTcpSocket::disconnected, this, &FakeEwsConnection::disconnected);
    connect(mSock.data(), &QTcpSocket::readyRead, this, &FakeEwsConnection::dataAvailable);
    connect(&mDataTimer, &QTimer::timeout, this, &FakeEwsConnection::dataTimeout);
}

FakeEwsConnection::~FakeEwsConnection()
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Connection closed.");
}

void FakeEwsConnection::disconnected()
{
    deleteLater();
}

void FakeEwsConnection::dataAvailable()
{
    if (mContentLength == 0) {
        QByteArray line = mSock->readLine();
        QList<QByteArray> tokens = line.split(' ');
        mKeepAlive = false;

        if (tokens.size() < 3) {
            sendError(QLatin1String("Invalid request header"));
            return;
        }
        if (tokens.at(0) != "POST") {
            sendError(QLatin1String("Expected POST request"));
            return;
        }
        if (tokens.at(1) != "/EWS/Exchange.asmx") {
            sendError(QLatin1String("Invalid EWS URL"));
            return;
        }

        do {
            line = mSock->readLine().trimmed();
            if (line.toLower().startsWith("content-length: ")) {
                bool ok;
                mContentLength = line.mid(16).toUInt(&ok);
                if (!ok) {
                    sendError(QLatin1String("Failed to parse content length."));
                    return;
                }
            } else if (line.toLower() == "connection: keep-alive") {
                mKeepAlive = true;
            }
        } while (!line.isEmpty());

        if (mContentLength == 0) {
            sendError(QLatin1String("Expected content"));
            return;
        }
    }

    mContent = mSock->read(mContentLength - mContent.size());

    if (mContent.size() >= static_cast<int>(mContentLength)) {
        mDataTimer.stop();

        FakeEwsServer::DialogEntry::HttpResponse resp = mReplyCallback(QString::fromUtf8(mContent));

        QByteArray buffer;
        QLatin1String codeStr = responseCodes.value(resp.second);
        QByteArray respContent = resp.first.toUtf8();
        buffer += QStringLiteral("HTTP/1.1 %1 %2\r\n").arg(resp.second).arg(codeStr).toLatin1();
        buffer += mKeepAlive ? "Connection: Keep-Alive\n" : "Connection: Close\r\n";
        buffer += "Content-Length: " + QByteArray::number(respContent.size()) + "\r\n";
        buffer += "\r\n";
        buffer += respContent;
        mSock->write(buffer);

        if (!mKeepAlive) {
            mSock->disconnectFromHost();
        }
    } else {
        mDataTimer.start(3000);
    }
}

void FakeEwsConnection::sendError(const QString &msg, ushort code)
{
    qCWarningNC(EWSFAKE_LOG) << msg;
    QLatin1String codeStr = responseCodes.value(code);
    QByteArray response(QStringLiteral("HTTP/1.1 %1 %2\nConnection: close\n\n").arg(code).arg(codeStr).toLatin1());
    response += msg.toLatin1();
    mSock->write(response);
    mSock->disconnectFromHost();
}

void FakeEwsConnection::dataTimeout()
{
    qCWarning(EWSFAKE_LOG) << QLatin1String("Timeout waiting for content.");
    sendError(QLatin1String("Timeout waiting for content."));
}
