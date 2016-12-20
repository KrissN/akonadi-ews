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

#define XML_ERROR_POS(reader, msg, code) \
    sendError(msg + QStringLiteral(" at character offset %1").arg(reader.characterOffset()), code)

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
    : QObject(parent), mSock(sock), mReplyCallback(replyCallback), mContentLength(0)
{
    connect(mSock.data(), &QTcpSocket::disconnected, this, &FakeEwsConnection::disconnected);
    connect(mSock.data(), &QTcpSocket::readyRead, this, &FakeEwsConnection::dataAvailable);
    connect(&mDataTimer, &QTimer::timeout, this, &FakeEwsConnection::dataTimeout);
}

FakeEwsConnection::~FakeEwsConnection()
{
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
        buffer += QStringLiteral("HTTP/1.1 %1 %2\n").arg(resp.second).arg(codeStr).toLatin1();
        buffer += "Connection: close\n";
        buffer += "\n";
        buffer += resp.first.toUtf8();
        mSock->write(buffer);

        mSock->disconnectFromHost();
    } else {
        mDataTimer.start(3000);
    }
}

void FakeEwsConnection::sendError(const QString &msg, ushort code)
{
    QLatin1String codeStr = responseCodes.value(code);
    QByteArray response(QStringLiteral("HTTP/1.1 %1 %2\nConnection: close\n\n").arg(code).arg(codeStr).toLatin1());
    response += msg.toLatin1();
    mSock->write(response);
    mSock->disconnectFromHost();
}

void FakeEwsConnection::dataTimeout()
{
    sendError(QLatin1String("Timeout waiting for content."));
}




/*


void FakeEwsConnection::dataReadyRead()
{
    QByteArray line = mSock->readLine();
    QList<QByteArray> tokens = line.split(' ');
    uint contentLength = 0;

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
            contentLength = line.mid(16).toUInt(&ok);
            if (!ok) {
                sendError(QLatin1String("Failed to parse content length."));
                return;
            }
        }
    } while (!line.isEmpty());

    if (contentLength == 0) {
        sendError(QLatin1String("Expected content"));
        return;
    }

    QByteArray content = mSock->read(contentLength);

    parseRequest(content);
}

void FakeEwsConnection::parseRequest(const QByteArray &req)
{
    QXmlStreamReader reader(req);

    if (!reader.readNextStartElement() || !SOAP_ELEMENT(reader, "Envelope")) {
        XML_ERROR_POS(reader, QStringLiteral("Expected SOAP Envelope element '%1'").arg(reader.name().toString()), 400);
        return;
    }

    if (!reader.readNextStartElement()) {
        XML_ERROR_POS(reader, QStringLiteral("Expected element inside SOAP Envelope"), 400);
        return;
    }

    if (SOAP_ELEMENT(reader, "Header")) {
        if (reader.readNextStartElement()) {
            if (EWS_TYPES_ELEMENT(reader, "ServerVersion")) {
                FakeEwsServerVersion ver(reader);

                if (!ver.isValid()) {
                    XML_ERROR_POS(reader, QStringLiteral("Invalid server version"), 400);
                    return;
                } else if (ver > mServerVersion) {
                    XML_ERROR_POS(reader, QStringLiteral("Unsupported server version"), 400);
                    return;
                }
                mServerVersion = ver;
            } else {
                XML_ERROR_POS(reader, QStringLiteral("Unknown header element \"%1\"").arg(reader.name().toString()), 400);
                return;
            }
            reader.skipCurrentElement();
        }
        reader.skipCurrentElement();
    }

    if (!SOAP_ELEMENT(reader, "Body")) {
        XML_ERROR_POS(reader, QStringLiteral("Unknown element '%1' inside SOAP envelope").arg(reader.name().toString()), 400);
        return;
    }

    if (!reader.readNextStartElement()) {
        XML_ERROR_POS(reader, QStringLiteral("Expected element inside SOAP Body"), 400);
        return;
    }

    if (reader.namespaceUri() != ewsMsgNsUri) {
        XML_ERROR_POS(reader, QStringLiteral("Expected valid EWS request element"), 400);
        return;
    }

    FakeEwsAbstractRequest *request = FakeEwsAbstractRequest::createRequest(reader.name().toString(), reader, *this);
    if (!request) {
        XML_ERROR_POS(reader, QStringLiteral("Unhandled EWS request '%1'").arg(reader.name().toString()), 400);
        return;
    } else {
        request->processRequest();
    }
}

void FakeEwsConnection::sendError(const QString &msg, ushort code)
{
    QLatin1String codeStr = responseCodes.value(code);
    QByteArray response(QStringLiteral("HTTP/1.1 %1 %2\nConnection: close\n\n").arg(code).arg(codeStr).toLatin1());
    response += msg.toLatin1();
    mSock->write(response);
    mSock->disconnectFromHost();
}

void FakeEwsConnection::sendResponse(const QByteArray &resp, bool closeConn)
{
    QByteArray buffer;
    buffer += "HTTP/1.1 200 OK\n";
    buffer += "Connection: ";
    buffer += closeConn ? "close\n" : "keep-alive\n";
    buffer += "\n";
    buffer += resp;
    mSock->write(buffer);

    if (closeConn) {
        mSock->disconnectFromHost();
    }
}*/
