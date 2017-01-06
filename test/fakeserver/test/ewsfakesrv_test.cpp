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

#include <QtTest/QtTest>
#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "fakeewsserver.h"

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
    void regexpResponse();
    void conditionalResponse();
    void emptyResponse();
    void getEventsRequest();
    void getEventsRequest_data();
    void getStreamingEventsRequest();
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

    QCOMPARE(respCode, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::invalidMethod()
{
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    QVERIFY(srv->start());

    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/some/url").arg(srv->portNumber()));
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
    srv->setDefaultReplyCallback([&receivedReq](const QString &req) {
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
            QStringLiteral("samplereq1"),
            QRegularExpression(),
            {QStringLiteral("sampleresp1"), 200},
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("samplereq1"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("sampleresp1"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

void UtEwsFakeSrvTest::callbackResponse()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("samplereq1"),
            QRegularExpression(),
            {},
            [](const QString &) {
                return FakeEwsServer::DialogEntry::HttpResponse("sampleresp2", 200);
            },
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("samplereq1"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("sampleresp2"));
    QCOMPARE(resp.second, static_cast<ushort>(200));
}

void UtEwsFakeSrvTest::regexpResponse()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QString(),
            QRegularExpression(QStringLiteral("samplereq[24]")),
            {QStringLiteral("sampleresp1"), 200},
            FakeEwsServer::DialogEntry::ReplyCallback(),
            QStringLiteral("Sample request 1")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("samplereq1"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));

    resp = synchronousHttpReq(QStringLiteral("samplereq2"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(200));
    QCOMPARE(resp.first, QStringLiteral("sampleresp1"));

    resp = synchronousHttpReq(QStringLiteral("samplereq3"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));

    resp = synchronousHttpReq(QStringLiteral("samplereq4"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(200));
    QCOMPARE(resp.first, QStringLiteral("sampleresp1"));
}

void UtEwsFakeSrvTest::conditionalResponse()
{
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QString(),
            QRegularExpression("samplereq[0-9]"),
            FakeEwsServer::EmptyResponse,
            [](const QString& req) {
                if (req[9] == '1' || req[9] == '3')
                    return FakeEwsServer::DialogEntry::HttpResponse(QStringLiteral("sampleresp1"), 200);
                else
                    return FakeEwsServer::EmptyResponse;
            },
            QStringLiteral("Sample request 1")
        },
        {
            QString(),
            QRegularExpression("samplereq[0-9]"),
            FakeEwsServer::EmptyResponse,
            [](const QString& req) {
            if (req[9] == '2' || req[9] == '3')
                return FakeEwsServer::DialogEntry::HttpResponse(QStringLiteral("sampleresp2"), 200);
            else
                return FakeEwsServer::EmptyResponse;
            },
            QStringLiteral("Sample request 2")
        }
    };

    QString receivedReq;
    QScopedPointer<FakeEwsServer> srv(new FakeEwsServer(this));
    srv->setDialog(dialog);
    QVERIFY(srv->start());

    auto resp = synchronousHttpReq(QStringLiteral("samplereq1"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("sampleresp1"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("samplereq2"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("sampleresp2"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("samplereq3"), srv->portNumber());
    QCOMPARE(resp.first, QStringLiteral("sampleresp1"));
    QCOMPARE(resp.second, static_cast<ushort>(200));

    resp = synchronousHttpReq(QStringLiteral("samplereq4"), srv->portNumber());
    QCOMPARE(resp.second, static_cast<ushort>(500));
}

void UtEwsFakeSrvTest::emptyResponse()
{
    bool callbackCalled = false;
    const FakeEwsServer::DialogEntry::List dialog = {
        {
            QStringLiteral("samplereq1"),
            QRegularExpression(),
            FakeEwsServer::EmptyResponse,
            [&callbackCalled](const QString &) {
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

    auto resp = synchronousHttpReq(QStringLiteral("samplereq1"), srv->portNumber());
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
            "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d</SubscriptionId>"
            "<Timeout>1</Timeout>"
            "</GetStreamingEvents>"
            "</soap:Body>"
            "</soap:Envelope>");

    srv->queueEventsXml(QStringList() << event);

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
                "<m:GetStreamingEventsResponse xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                "<m:ResponseMessages>"
                "<m:GetStreamingEventsResponseMessage ResponseClass=\"Success\">"
                "<m:ResponseCode>NoError</m:ResponseCode>"
                "<m:Notification>"
                "<SubscriptionId>f6bc657d-dde1-4f94-952d-143b95d6483d<SubscriptionId>");
        const QString respTail = QStringLiteral("</m:Notification>"
                "</m:GetStreamingEventsResponseMessage>"
                "</m:ResponseMessages>"
                "</m:GetStreamingEventsResponse>"
                "</soap:Body>"
                "</soap:Envelope>");
        const QString event2 = QStringLiteral("<NewMailEvent>"
                    "<TimeStamp>2006-08-22T01:00:50Z</TimeStamp>"
                    "<ItemId Id=\"AQApAHRw\" ChangeKey=\"CQAAAA==\" />"
                    "<ParentFolderId Id=\"AQApAH\" ChangeKey=\"AQAAAA==\" />"
                    "</NewMailEvent>");
        callbackCalled = true;

        QString expResp = respHead;
        if (responseNo == 0) {
            expResp += event;
        } else if (responseNo == 2) {
            expResp += event2;
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

    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = startTime.msecsTo(now);
    qDebug() << elapsed;
    QVERIFY(elapsed >= 1 * 60 * 1000 - 600);
    QVERIFY(elapsed <= 1 * 60 * 1000 + 600);
}

QPair<QString, ushort> UtEwsFakeSrvTest::synchronousHttpReq(const QString &content, ushort port,
                                                            std::function<bool(const QString &)> chunkFn)
{
    QNetworkAccessManager nam(this);
    QUrl url(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(port));
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
