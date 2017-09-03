/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include <QBuffer>
#include <QTcpSocket>
#include <QXmlNamePool>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QXmlSerializer>

#include "fakeewsserver.h"
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

static Q_CONSTEXPR int streamingEventsHeartbeatIntervalSeconds = 5;

FakeEwsConnection::FakeEwsConnection(QTcpSocket *sock, FakeEwsServer* parent)
    : QObject(parent), mSock(sock), mContentLength(0), mKeepAlive(false), mState(Initial),
      mAuthenticated(false)
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got new EWS connection.");
    connect(mSock.data(), &QTcpSocket::disconnected, this, &FakeEwsConnection::disconnected);
    connect(mSock.data(), &QTcpSocket::readyRead, this, &FakeEwsConnection::dataAvailable);
    connect(&mDataTimer, &QTimer::timeout, this, &FakeEwsConnection::dataTimeout);
    connect(&mStreamingRequestHeartbeat, &QTimer::timeout, this, &FakeEwsConnection::streamingRequestHeartbeat);
    connect(&mStreamingRequestTimeout, &QTimer::timeout, this, &FakeEwsConnection::streamingRequestTimeout);
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
    if (mState == Initial) {
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
        mState = RequestReceived;
    }

    if (mState == RequestReceived) {
        QByteArray line;
        do {
            line = mSock->readLine();
            if (line.toLower().startsWith(QLatin1String("content-length: "))) {
                bool ok;
                mContentLength = line.trimmed().mid(16).toUInt(&ok);
                if (!ok) {
                    sendError(QLatin1String("Failed to parse content length."));
                    return;
                }
            } else if (line.toLower().startsWith(QLatin1String("authorization: basic "))) {
                if (line.trimmed().mid(21) == "dGVzdDp0ZXN0") {
                    mAuthenticated = true;
                }
            } else if (line.toLower() == "connection: keep-alive\r\n") {
                mKeepAlive = true;
            }
        } while (!line.trimmed().isEmpty());

        if (line == "\r\n") {
            mState = HeadersReceived;
        }
    }

    if (mState == HeadersReceived) {
        if (mContentLength == 0) {
            sendError(QLatin1String("Expected content"));
            return;
        }

        mContent += mSock->read(mContentLength - mContent.size());

        if (mContent.size() >= static_cast<int>(mContentLength)) {
            mDataTimer.stop();

            if (!mAuthenticated) {
                QLatin1String codeStr = responseCodes.value(401);
                QByteArray response(QStringLiteral("HTTP/1.1 %1 %2\r\n"
                        "WWW-Authenticate: Basic realm=\"Fake EWS Server\"\r\n"
                        "Connection: close\r\n"
                        "\r\n").arg(401).arg(codeStr).toAscii());
                response += codeStr;
                mSock->write(response);
                mSock->disconnectFromHost();
                return;
            }

            FakeEwsServer::DialogEntry::HttpResponse resp = parseRequest(QString::fromUtf8(mContent));
            bool chunked = false;

            if (resp == FakeEwsServer::EmptyResponse) {
                resp = handleGetEventsRequest(mContent);
            }

            if (resp == FakeEwsServer::EmptyResponse) {
                resp = handleGetStreamingEventsRequest(mContent);
                if (resp.second > 1000) {
                    chunked = true;
                    resp.second %= 1000;
                }
            }

            FakeEwsServer *server = qobject_cast<FakeEwsServer*>(parent());
            auto defaultReplyCallback = server->defaultReplyCallback();
            if (defaultReplyCallback && (resp == FakeEwsServer::EmptyResponse)) {
                QXmlResultItems ri;
                QXmlNamePool namePool;
                resp = defaultReplyCallback(QString::fromUtf8(mContent), ri, namePool);
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning response from default callback ")
                        << resp.second << QStringLiteral(": ") << resp.first;
            }

            if (resp == FakeEwsServer::EmptyResponse) {
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning default response 500.");
                resp = { QStringLiteral(""), 500 };
            }

            QByteArray buffer;
            QLatin1String codeStr = responseCodes.value(resp.second);
            QByteArray respContent = resp.first.toUtf8();
            buffer += QStringLiteral("HTTP/1.1 %1 %2\r\n").arg(resp.second).arg(codeStr).toLatin1();
            if (chunked) {
                buffer += "Transfer-Encoding: chunked\r\n";
                buffer += "\r\n";
                buffer += QByteArray::number(respContent.size(), 16) + "\r\n";
                buffer += respContent + "\r\n";
            } else {
                buffer += "Content-Length: " + QByteArray::number(respContent.size()) + "\r\n";
                buffer += mKeepAlive ? "Connection: Keep-Alive\n" : "Connection: Close\r\n";
                buffer += "\r\n";
                buffer += respContent;
            }
            mSock->write(buffer);

            if (!mKeepAlive && !chunked) {
                mSock->disconnectFromHost();
            }
            mContent.clear();
            mState = Initial;
        } else {
            mDataTimer.start(3000);
        }
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

FakeEwsServer::DialogEntry::HttpResponse FakeEwsConnection::parseRequest(const QString &content)
{
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got request: ") << content;

    FakeEwsServer *server = qobject_cast<FakeEwsServer*>(parent());
    FakeEwsServer::DialogEntry::HttpResponse resp = FakeEwsServer::EmptyResponse;
    Q_FOREACH(const FakeEwsServer::DialogEntry &de, server->dialog()) {
        QXmlResultItems ri;
        QByteArray resultBytes;
        QString result;
        QBuffer resultBuffer(&resultBytes);
        resultBuffer.open(QIODevice::WriteOnly);
        QXmlQuery query;
        QXmlSerializer xser(query, &resultBuffer);
        if (!de.xQuery.isNull()) {
            query.setFocus(content);
            query.setQuery(de.xQuery);
            query.evaluateTo(&xser);
            query.evaluateTo(&ri);
            if (ri.hasError()) {
                qCDebugNC(EWSFAKE_LOG) << QStringLiteral("XQuery failed due to errors - skipping");
                continue;
            }
            result = QString::fromUtf8(resultBytes);
        }

        if (!result.trimmed().isEmpty()) {
            qCDebugNC(EWSFAKE_LOG) << QStringLiteral("Got match for \"") << de.description << QStringLiteral("\"");
            if (de.replyCallback) {
                resp = de.replyCallback(content, ri, query.namePool());
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning response from callback ")
                        << resp.second << QStringLiteral(": ") << resp.first;
            } else {
                resp = {result.trimmed(), 200};
                qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning response from XQuery ")
                        << resp.second << QStringLiteral(": ") << resp.first;
            }
            break;
        }
    }

    if (resp == FakeEwsServer::EmptyResponse) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning empty response.");
        qCInfoNC(EWSFAKE_LOG) << content;
    }

    return resp;
}

FakeEwsServer::DialogEntry::HttpResponse FakeEwsConnection::handleGetEventsRequest(const QString &content)
{
    const QRegularExpression re(QStringLiteral("<?xml .*<\\w*:?GetEvents[ >].*<\\w*:?SubscriptionId>(?<subid>[^<]*)</\\w*:?SubscriptionId><\\w*:?Watermark>(?<watermark>[^<]*)</\\w*:?Watermark></\\w*:?GetEvents>.*"));

    QRegularExpressionMatch match = re.match(content);
    if (!match.hasMatch() || match.hasPartialMatch()) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Not a valid GetEvents request.");
        return FakeEwsServer::EmptyResponse;
    }

    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got valid GetEvents request.");

    QString resp = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
            "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
            "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
            "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
            "<soap:Header>"
            "<t:ServerVersionInfo MajorVersion=\"8\" MinorVersion=\"0\" MajorBuildNumber=\"628\" MinorBuildNumber=\"0\" />"
            "</soap:Header>"
            "<soap:Body>"
            "<m:GetEventsResponse xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
            "<m:ResponseMessages>"
            "<m:GetEventsResponseMessage ResponseClass=\"Success\">"
            "<m:ResponseCode>NoError</m:ResponseCode>"
            "<m:Notification>");

    if (match.captured("subid").isEmpty() || match.captured("watermark").isEmpty()) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Missing subscription id or watermark.");
        const QString errorResp = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
                "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Header>"
                "<t:ServerVersionInfo MajorVersion=\"8\" MinorVersion=\"0\" MajorBuildNumber=\"628\" MinorBuildNumber=\"0\" />"
                "</soap:Header>"
                "<soap:Body>"
                "<m:GetEventsResponse>"
                "<m:ResponseMessages>"
                "<m:GetEventsResponseMessage ResponseClass=\"Error\">"
                "<m:MessageText>Missing subscription id or watermark.</m:MessageText>"
                "<m:ResponseCode>ErrorInvalidPullSubscriptionId</m:ResponseCode>"
                "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                "</m:GetEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");
        return {errorResp, 200};
    }

    resp += QStringLiteral("<SubscriptionId>") + match.captured("subid") + QStringLiteral("<SubscriptionId>");
    resp += QStringLiteral("<PreviousWatermark>") + match.captured("watermark") + QStringLiteral("<PreviousWatermark>");
    resp += QStringLiteral("<MoreEvents>false<MoreEvents>");

    FakeEwsServer *server = qobject_cast<FakeEwsServer*>(parent());
    const QStringList events = server->retrieveEventsXml();
    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning %1 events.").arg(events.size());
    Q_FOREACH(const QString &eventXml, events) {
        resp += eventXml;
    }

    resp += QStringLiteral("</m:Notification></m:GetEventsResponseMessage></m:ResponseMessages>"
            "</m:GetEventsResponse></soap:Body></soap:Envelope>");


    return {resp, 200};
}

QString FakeEwsConnection::prepareEventsResponse(const QStringList &events)
{
    QString resp = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
            "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
            "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
            "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
            "<soap:Header>"
            "<t:ServerVersionInfo MajorVersion=\"8\" MinorVersion=\"0\" MajorBuildNumber=\"628\" MinorBuildNumber=\"0\" />"
            "</soap:Header>"
            "<soap:Body>"
            "<m:GetStreamingEventsResponse>"
            "<m:ResponseMessages>"
            "<m:GetStreamingEventsResponseMessage ResponseClass=\"Success\">"
            "<m:ResponseCode>NoError</m:ResponseCode>"
            "<m:ConnectionStatus>OK</m:ConnectionStatus>");

    if (!events.isEmpty()) {
        resp += QStringLiteral("<m:Notifications><m:Notification><SubscriptionId>") + mStreamingSubId + QStringLiteral("<SubscriptionId>");

        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Returning %1 events.").arg(events.size());
        Q_FOREACH(const QString &eventXml, events) {
            resp += eventXml;
        }

        resp += QStringLiteral("</m:Notification></m:Notifications>");
    }
    resp += QStringLiteral("</m:GetStreamingEventsResponseMessage></m:ResponseMessages>"
            "</m:GetStreamingEventsResponse></soap:Body></soap:Envelope>");

    return resp;
}

FakeEwsServer::DialogEntry::HttpResponse FakeEwsConnection::handleGetStreamingEventsRequest(const QString &content)
{
    const QRegularExpression re(QStringLiteral("<?xml .*<\\w*:?GetStreamingEvents[ >].*<\\w*:?SubscriptionIds><\\w*:?SubscriptionId>(?<subid>[^<]*)</\\w*:?SubscriptionId></\\w*:?SubscriptionIds>.*<\\w*:?ConnectionTimeout>(?<timeout>[^<]*)</\\w*:?ConnectionTimeout></\\w*:?GetStreamingEvents>.*"));

    QRegularExpressionMatch match = re.match(content);
    if (!match.hasMatch() || match.hasPartialMatch()) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Not a valid GetStreamingEvents request.");
        return FakeEwsServer::EmptyResponse;
    }

    qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Got valid GetStreamingEvents request.");

    if (match.captured("subid").isEmpty() || match.captured("timeout").isEmpty()) {
        qCInfoNC(EWSFAKE_LOG) << QStringLiteral("Missing subscription id or timeout.");
        const QString errorResp = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
                "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Header>"
                "<t:ServerVersionInfo MajorVersion=\"8\" MinorVersion=\"0\" MajorBuildNumber=\"628\" MinorBuildNumber=\"0\" />"
                "</soap:Header>"
                "<soap:Body>"
                "<m:GetStreamingEventsResponse>"
                "<m:ResponseMessages>"
                "<m:GetStreamingEventsResponseMessage ResponseClass=\"Error\">"
                "<m:MessageText>Missing subscription id or timeout.</m:MessageText>"
                "<m:ResponseCode>ErrorInvalidSubscription</m:ResponseCode>"
                "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                "</m:GetEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");
        return {errorResp, 200};
    }

    mStreamingSubId = match.captured("subid");

    FakeEwsServer *server = qobject_cast<FakeEwsServer*>(parent());
    const QStringList events = server->retrieveEventsXml();

    QString resp = prepareEventsResponse(events);

    mStreamingRequestTimeout.start(match.captured("timeout").toInt() * 1000 * 60);
    mStreamingRequestHeartbeat.setSingleShot(false);
    mStreamingRequestHeartbeat.start(streamingEventsHeartbeatIntervalSeconds * 1000);

    Q_EMIT streamingRequestStarted(this);

    return {resp, 1200};
}

void FakeEwsConnection::streamingRequestHeartbeat()
{
    sendEvents(QStringList());
}

void FakeEwsConnection::streamingRequestTimeout()
{
    mStreamingRequestTimeout.stop();
    mStreamingRequestHeartbeat.stop();
    mSock->write("0\r\n\r\n");
    mSock->disconnectFromHost();
}

void FakeEwsConnection::sendEvents(const QStringList &events)
{
    QByteArray resp = prepareEventsResponse(events).toUtf8();

    mSock->write(QByteArray::number(resp.size(), 16) + "\r\n" + resp + "\r\n");
}

