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

#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlItem>
#include <QXmlNamePool>
#include <QXmlResultItems>
#include <QtTest>

#include "fakeewsserver.h"
#include "fakeewsserverthread.h"

class UtEwsFakeSrvTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void emptyDialog();
    void invalidURL();
    void invalidMethod();
    void emptyRequest();
    void defaultCallback();
    void simpleResponse();
    void callbackResponse();
    void multipleResponses();
    void emptyResponse();
    void getEventsRequest();
    void getEventsRequest_data();
    void getStreamingEventsRequest();
    void serverThread();
    void delayedContentSize();
    void notAuthenticated();
    void badAuthentication();
    void xqueryResultsInCallback();
private:
    QPair<QString, ushort> synchronousHttpReq(const QString &content, ushort port,
                                              std::function<bool(const QString &)> chunkFn = Q_NULLPTR);
};

void UtEwsFakeSrvTest::emptyDialog()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(srv->portNumber()));
    url.setUserName("test");
    url.setPassword("test");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    QNetworkReply *reply = nam.post(req, "test");

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp]() {
        resp += QString::fromUtf8(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    QCOMPARE(respCode, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::invalidURL()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/some/url").arg(srv->portNumber()));
    url.setUserName("test");
    url.setPassword("test");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    QNetworkReply *reply = nam.post(req, "test");

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp]() {
        resp += QString::fromUtf8(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    QCOMPARE(respCode, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::invalidMethod()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(srv->portNumber()));
    url.setUserName("test");
    url.setPassword("test");
    QNetworkRequest req(url);
    QNetworkReply *reply = nam.get(req);

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp]() {
        resp += QString::fromUtf8(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    QCOMPARE(respCode, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::emptyRequest()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral(""), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::defaultCallback()
{
    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDefaultReplyCallback([&receivedReq](const QString &req, QXmlResultItems &, const QXmlNamePool &) {
        receivedReq = req;
        return FakeEwsServer::DialogEntry::HttpResponse(QStringLiteral("testresp"), 200);
    });
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("testreq"), srv->portNumber());
    QCOMPARE(receivedReq, QStringLiteral("testreq"));
    QCOMPARE(resp.first, QStringLiteral("testresp"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

void UtEwsFakeSrvTest::simpleResponse()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a />) then (<b/>) else ()"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<b/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

void UtEwsFakeSrvTest::callbackResponse()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a />) then (<b/>) else ()"),
            [](const QString &, QXmlResultItems &, const QXmlNamePool &) {
                return FakeEwsServer::DialogEntry::HttpResponse("<a/>", 200);
            },
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<a/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

void UtEwsFakeSrvTest::multipleResponses()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a /> or //test1/b = <b />) then (<aresult/>) else ()"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 1")
        },
        {
            QStringLiteral("if (//test1/b = <b /> or //test1/c = <c />) then (<bresult/>) else ()"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 2")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<aresult/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("<test1><b /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<aresult/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("<test1><c /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<bresult/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("<test1><d /></test1>"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::emptyResponse()
{
    bool callbackCalled = false;
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a />) then (<b/>) else ()"),
            [&callbackCalled](const QString &, QXmlResultItems &, const QXmlNamePool &) {
                callbackCalled = true;
                return FakeEwsServer::EmptyResponse;
            },
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));
    QCOMPARE(callbackCalled, true);
}

void UtEwsFakeSrvTest::getEventsRequest()
{
    const FakeEwsServer::DialogEntry::List emptyDialog;

    const QFETCH(QString, request);
    const QFETCH(QStringList, events);
    const QFETCH(ushort, responseCode);
    const QFETCH(QString, response);

    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    srv->queueEventsXml(events);

    auto resp = synchronousHttpReq(request, srv->portNumber());
    QCOMPARE(resp.second, responseCode);
    if (responseCode == 200) {
        QCOMPARE(resp.first, response);
    }
}

void UtEwsFakeSrvTest::getEventsRequest_data()
{
    QTest::addColumn<QString>("request");
    QTest::addColumn<QStringList>("events");
    QTest::addColumn<ushort>("responseCode");
    QTest::addColumn<QString>("response");

    QTest::newRow("valid request (MSDN)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Body>"
                "<GetEvents xmlns=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</SubscriptionId>"
                "<Watermark>AAAAAMAGAAAAAAAAAQ==</Watermark>"
                "</GetEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << (QStringList()
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAM4GAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T00:36:29Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHR\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>")
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>"))
        << static_cast<ushort>(200)
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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
                "<m:ResponseCode>NoError</m:ResponseCode><m:Notification>"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d<SubscriptionId>"
                "<PreviousWatermark>AAAAAMAGAAAAAAAAAQ==<PreviousWatermark>"
                "<MoreEvents>false<MoreEvents>"
                "<NewMailEvent>"
                "<Watermark>AAAAAM4GAAAAAAAAAQ==</Watermark>"
                "<TimeStamp>2006-08-22T00:36:29Z</TimeStamp>"
                "<ItemId Id=\"AQApAHR\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>"
                "<NewMailEvent>"
                "<Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>"
                "</m:Notification>"
                "</m:GetEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");

    QTest::newRow("valid request (namespaced)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
                "<soap:Body>"
                "<m:GetEvents>"
                "<m:SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</m:SubscriptionId>"
                "<m:Watermark>AAAAAMAGAAAAAAAAAQ==</m:Watermark>"
                "</m:GetEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << (QStringList()
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAM4GAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T00:36:29Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHR\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>")
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>"))
        << static_cast<ushort>(200)
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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
                "<m:ResponseCode>NoError</m:ResponseCode><m:Notification>"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d<SubscriptionId>"
                "<PreviousWatermark>AAAAAMAGAAAAAAAAAQ==<PreviousWatermark>"
                "<MoreEvents>false<MoreEvents>"
                "<NewMailEvent>"
                "<Watermark>AAAAAM4GAAAAAAAAAQ==</Watermark>"
                "<TimeStamp>2006-08-22T00:36:29Z</TimeStamp>"
                "<ItemId Id=\"AQApAHR\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>"
                "<NewMailEvent>"
                "<Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>"
                "</m:Notification>"
                "</m:GetEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");

    QTest::newRow("invalid request (missing subscription id)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Body>"
                "<GetEvents xmlns=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
                "<SubscriptionId></SubscriptionId>"
                "<Watermark>AAAAAMAGAAAAAAAAAQ==</Watermark>"
                "</GetEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << (QStringList()
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>"))
        << static_cast<ushort>(200)
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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

    QTest::newRow("invalid request (missing watermark)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Body>"
                "<GetEvents xmlns=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</SubscriptionId>"
                "<Watermark></Watermark>"
                "</GetEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << (QStringList()
            << QStringLiteral("<NewMailEvent><Watermark>AAAAAOQGAAAAAAAAAQ==</Watermark>"
                    "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>"))
        << static_cast<ushort>(200)
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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

    QTest::newRow("valid request (no events)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Body>"
                "<GetEvents xmlns=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</SubscriptionId>"
                "<Watermark>AAAAAMAGAAAAAAAAAQ==</Watermark>"
                "</GetEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << QStringList()
        << static_cast<ushort>(200)
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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
                "<m:ResponseCode>NoError</m:ResponseCode><m:Notification>"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d<SubscriptionId>"
                "<PreviousWatermark>AAAAAMAGAAAAAAAAAQ==<PreviousWatermark>"
                "<MoreEvents>false<MoreEvents>"
                "</m:Notification>"
                "</m:GetEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");

    QTest::newRow("invalid request (not a GetEvents request)")
        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<soap:Body>"
                "<m:GetStreamingEvents>"
                "<m:SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</m:SubscriptionId>"
                "<m:ConnectionTimeout>30</m:ConnectionTimeout>"
                "</m:GetStreamingEvents>"
                "</soap:Body>"
                "</soap:Envelope>")
        << QStringList()
        << static_cast<ushort>(500)
        << QString();
}

void UtEwsFakeSrvTest::getStreamingEventsRequest()
{
    bool callbackCalled = false;
    const FakeEwsServer::DialogEntry::List emptyDialog;

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());
    QDateTime startTime = QDateTime::currentDateTime();

    const QString event = QStringLiteral("<NewMailEvent>"
                "<TimeStamp>2006-08-22T01:00:10Z</TimeStamp>"
                "<ItemId Id=\"AQApAHRx\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAc\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>");
    const QString content = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
            "<soap:Body>"
            "<GetStreamingEvents xmlns=\"http://schemas.microsoft.com/exchange/services/2006/messages\">"
            "<SubscriptionIds><SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</SubscriptionId></SubscriptionIds>"
            "<ConnectionTimeout>1</ConnectionTimeout>"
            "</GetStreamingEvents>"
            "</soap:Body>"
            "</soap:Envelope>");

    srv->queueEventsXml(QStringList() << event);

    bool unknownRequestEncountered = false;
    srv->setDefaultReplyCallback([&](const QString &req, QXmlResultItems &, const QXmlNamePool &) {
        qDebug() << "Unknown EWS request encountered." << req;
        unknownRequestEncountered = true;
        return FakeEwsServer::EmptyResponse;
    });

    bool callbackError = false;
    int responseNo = 0;
    QDateTime pushEventTime;
    auto resp = synchronousHttpReq(content, srv->portNumber(), [&](const QString &chunk) {
        const QString respHead = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
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
        const QString respTail = QStringLiteral("</m:GetStreamingEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetStreamingEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");
        const QString eventHead = QStringLiteral("<m:Notifications>"
                "<m:Notification>"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d<SubscriptionId>");
        const QString eventTail = QStringLiteral("</m:Notification></m:Notifications>");
        const QString event2 = QStringLiteral("<NewMailEvent>"
                "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                "</NewMailEvent>");
        callbackCalled = true;

        QString expResp = respHead;
        if (responseNo == 0) {
            expResp += eventHead + event + eventTail;
        } else if (responseNo == 2) {
            expResp += eventHead + event2 + eventTail;
        }
        expResp += respTail;

        if (chunk != expResp) {
            qWarning() << "Unexpected GetStreamingEventsResponse";
            callbackError = true;
            return false;
        }

        if (responseNo == 1) {
            srv->queueEventsXml(QStringList() << event2);
            pushEventTime = QDateTime::currentDateTime();
        } else if (responseNo == 2) {
            /* Check if the response to the above event came "immediatelly" */
            QDateTime now = QDateTime::currentDateTime();
            if (pushEventTime.msecsTo(now) > 200) {
                qWarning() << "Push event maximum roundtrip time exceeded";
                callbackError = true;
                return false;
            }
        }

        responseNo++;
        return true;
    });

    QCOMPARE(callbackCalled, true);
    QCOMPARE(resp.second, static_cast<ushort>(200));
    QCOMPARE(callbackError, false);

    QCOMPARE(unknownRequestEncountered, false);

    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = startTime.msecsTo(now);
    qDebug() << elapsed;
    QVERIFY(elapsed >= 1 * 60 * 1000 - 600);
    QVERIFY(elapsed <= 1 * 60 * 1000 + 600);
}

void UtEwsFakeSrvTest::serverThread()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
             QStringLiteral("if (//test1/a = <a />) then (<b/>) else ()"),
             FakeEwsServer::DialogEntry::ReplyCallback(),
             QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    FakeEwsServerThread thread;
    thread.start();
    QVERIFY(thread.waitServerStarted());
    thread.setDialog(dialog);

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), thread.portNumber());
    QCOMPARE(resp.first, QStringLiteral("<b/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    thread.exit();
    thread.wait();
}

void UtEwsFakeSrvTest::delayedContentSize()
{
    /* This test case simulates the behaviour of KIO HTTP, which sends the data in three chunks:
     *  - initial headers
     *  - Content-Length header + end of headers
     *  - content
     */

    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a />) then (<b/>) else ()"),
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    FakeEwsServerThread thread;
    thread.start();
    QVERIFY(thread.waitServerStarted());
    thread.setDialog(dialog);

    QTcpSocket sock;
    sock.connectToHost(QHostAddress(QHostAddress::LocalHost), thread.portNumber());
    QVERIFY(sock.waitForConnected(1000));
    sock.write(QStringLiteral("POST /EWS/Exchange.asmx HTTP/1.1\r\n"
            "Host: 127.0.0.1:%1\r\n"
            "Connection: keep-alive\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) KHTML/5.26.0 (like Gecko) Konqueror/5.26\r\n"
            "Pragma: no-cache\r\n"
            "Cache-control: no-cache\r\n"
            "Accept: text/html, text/*;q=0.9, image/jpeg;q=0.9, image/png;q=0.9, image/*;q=0.9, */*;q=0.8\r\n"
            "Accept-Charset: utf-8,*;q=0.5\r\n"
            "Accept-Language: pl-PL,en;q=0.9\r\n"
            "Authorization: Basic dGVzdDp0ZXN0\r\n"
            "Content-Type: text/xml\r\n").arg(thread.portNumber()).toAscii());
    sock.waitForBytesWritten(100);
    QThread::msleep(100);
    sock.write("Content-Length: 20\r\n\r\n");
    sock.waitForBytesWritten(100);
    QThread::msleep(100);
    sock.write("<test1><a /></test1>");
    sock.waitForBytesWritten(100);
    sock.waitForReadyRead(300);

    QByteArray data = sock.readAll();

    thread.exit();
    thread.wait();

    QCOMPARE(data, QByteArray("HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: Keep-Alive\n\r\n<b/>"));
}

void UtEwsFakeSrvTest::notAuthenticated()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(srv->portNumber()));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    QNetworkReply *reply = nam.post(req, "test");

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp]() {
        resp += QString::fromUtf8(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    QCOMPARE(respCode, static_cast<ushort>(401));
}

void UtEwsFakeSrvTest::badAuthentication()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(srv->portNumber()));
    url.setUserName("foo");
    url.setPassword("bar");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    QNetworkReply *reply = nam.post(req, "test");

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp]() {
        resp += QString::fromUtf8(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    QCOMPARE(respCode, static_cast<ushort>(401));
}

void UtEwsFakeSrvTest::xqueryResultsInCallback()
{
    bool callbackOk = false;
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("if (//test1/a = <a />) then (<b>test</b>) else ()"),
            [&callbackOk](const QString &, QXmlResultItems &ri, const QXmlNamePool &) {
                if (ri.hasError()) {
                    qDebug() << "XQuery result has errors.";
                    return FakeEwsServer::EmptyResponse;
                }
                QXmlItem item = ri.next();
                if (item.isNull()) {
                    qDebug() << "XQuery result has no items.";
                    return FakeEwsServer::EmptyResponse;
                }
                if (!item.isNode()) {
                    qDebug() << "XQuery result is not a XML node.";
                    return FakeEwsServer::EmptyResponse;
                }
                QXmlNodeModelIndex index = item.toNodeModelIndex();
                if (index.isNull()) {
                    qDebug() << "XQuery XML node is NULL.";
                    return FakeEwsServer::EmptyResponse;
                }
                if (index.model()->stringValue(index) != QStringLiteral("test")) {
                    qDebug() << "Unexpected item value:" << index.model()->stringValue(index);
                    return FakeEwsServer::EmptyResponse;
                }
                return FakeEwsServer::DialogEntry::HttpResponse("<b/>", 200);
            },
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("<test1><a /></test1>"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("<b/>"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

QPair<QString, ushort> UtEwsFakeSrvTest::synchronousHttpReq(const QString &content, ushort port,
                                                            std::function<bool(const QString &)> chunkFn)
{
    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(port));
    url.setUserName("test");
    url.setPassword("test");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    QNetworkReply *reply = nam.post(req, content.toUtf8());

    QEventLoop loop;
    QString resp;
    ushort respCode = 0;

    connect(reply, &QNetworkReply::readyRead, this, [reply, &resp, &chunkFn, &loop]() {
        QString chunk = QString::fromUtf8(reply->readAll());
        if (chunkFn) {
            bool cont = chunkFn(chunk);
            if (!cont) {
                reply->close();
                loop.exit(200);
                return;
            }
        } else {
            resp += chunk;
        }
    });
    connect(reply, &QNetworkReply::finished, this, [&loop, reply]() {
        loop.exit(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
    });
    respCode = loop.exec();

    return {resp, respCode};
}

QTEST_MAIN(UtEwsFakeSrvTest)

#include "ewsfakesrv_test.moc"
