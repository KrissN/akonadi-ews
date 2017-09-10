/*
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
#include <QtTest>

#include "fakehttppost.h"

#include "ewsdeleteitemrequest.h"

class UtEwsDeleteItemRequest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void singleItem();
    void twoItems();
    void twoItemsOneFailed();
    void twoItemsSecondFailed();
private:
    void verifier(FakeTransferJob* job, const QByteArray& req, const QByteArray &expReq,
                  const QByteArray &resp);

    EwsClient mClient;
};

void UtEwsDeleteItemRequest::singleItem()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:DeleteItem DeleteType=\"SoftDelete\">"
                    "<m:ItemIds><t:ItemId Id=\"+IRgnMJ8x+J6MQAZ\" ChangeKey=\"1iQt/At9\"/></m:ItemIds>"
                    "</m:DeleteItem></soap:Body></soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header>"
                    "<h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" "
                    "MajorBuildNumber=\"248\" MinorBuildNumber=\"2\" "
                    "Version=\"Exchange2007_SP1\" "
                    "xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"/>"
                    "</s:Header>"
                    "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
                    "<m:DeleteItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:DeleteItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:DeleteItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsDeleteItemRequest> req(new EwsDeleteItemRequest(mClient, this));
    EwsId::List ids;
    ids << EwsId("+IRgnMJ8x+J6MQAZ", "1iQt/At9");
    req->setItemIds(ids);
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 1);
    EwsDeleteItemRequest::Response resp = req->responses().first();
    QCOMPARE(resp.responseClass(), EwsResponseSuccess);
}

void UtEwsDeleteItemRequest::twoItems()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:DeleteItem DeleteType=\"SoftDelete\">"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"9LB1MiL3cOYUjmYy\" ChangeKey=\"TBjl3rnU\"/>"
                    "<t:ItemId Id=\"rZ0sc7Gfn9+XHVgv\" ChangeKey=\"pHTEe9nY\"/>"
                    "</m:ItemIds>"
                    "</m:DeleteItem></soap:Body></soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header>"
                    "<h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" "
                    "MajorBuildNumber=\"248\" MinorBuildNumber=\"2\" "
                    "Version=\"Exchange2007_SP1\" "
                    "xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"/>"
                    "</s:Header>"
                    "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
                    "<m:DeleteItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:DeleteItemResponseMessage>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:DeleteItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:DeleteItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsDeleteItemRequest> req(new EwsDeleteItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId("9LB1MiL3cOYUjmYy", "TBjl3rnU"),
        EwsId("rZ0sc7Gfn9+XHVgv", "pHTEe9nY")
    };
    req->setItemIds(ids);
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    Q_FOREACH(const EwsDeleteItemRequest::Response &resp, req->responses()) {
        QCOMPARE(resp.responseClass(), EwsResponseSuccess);
    }
}

void UtEwsDeleteItemRequest::twoItemsOneFailed()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:DeleteItem DeleteType=\"SoftDelete\">"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"9LB1MiL3cOYUjmYy\" ChangeKey=\"TBjl3rnU\"/>"
                    "<t:ItemId Id=\"rZ0sc7Gfn9+XHVgv\" ChangeKey=\"pHTEe9nY\"/>"
                    "</m:ItemIds>"
                    "</m:DeleteItem></soap:Body></soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header>"
                    "<h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" "
                    "MajorBuildNumber=\"248\" MinorBuildNumber=\"2\" "
                    "Version=\"Exchange2007_SP1\" "
                    "xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"/>"
                    "</s:Header>"
                    "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
                    "<m:DeleteItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Error\">"
                    "<m:MessageText>The specified object was not found in the store.</m:MessageText>"
                    "<m:ResponseCode>ErrorItemNotFound</m:ResponseCode>"
                    "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                    "</m:DeleteItemResponseMessage>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:DeleteItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:DeleteItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsDeleteItemRequest> req(new EwsDeleteItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId("9LB1MiL3cOYUjmYy", "TBjl3rnU"),
        EwsId("rZ0sc7Gfn9+XHVgv", "pHTEe9nY")
    };
    req->setItemIds(ids);
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    static const QList<EwsResponseClass> respClasses = {
        EwsResponseError,
        EwsResponseSuccess
    };
    QList<EwsResponseClass>::const_iterator respClassesIt = respClasses.begin();
    unsigned i = 0;
    Q_FOREACH(const EwsDeleteItemRequest::Response &resp, req->responses()) {
        qDebug() << "Verifying response" << i++;
        QCOMPARE(resp.responseClass(), *respClassesIt);
        respClassesIt++;
    }
}

void UtEwsDeleteItemRequest::twoItemsSecondFailed()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:DeleteItem DeleteType=\"SoftDelete\">"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"9LB1MiL3cOYUjmYy\" ChangeKey=\"TBjl3rnU\"/>"
                    "<t:ItemId Id=\"rZ0sc7Gfn9+XHVgv\" ChangeKey=\"pHTEe9nY\"/>"
                    "</m:ItemIds>"
                    "</m:DeleteItem></soap:Body></soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header>"
                    "<h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" "
                    "MajorBuildNumber=\"248\" MinorBuildNumber=\"2\" "
                    "Version=\"Exchange2007_SP1\" "
                    "xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"/>"
                    "</s:Header>"
                    "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
                    "<m:DeleteItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Error\">"
                    "<m:MessageText>The specified object was not found in the store.</m:MessageText>"
                    "<m:ResponseCode>ErrorItemNotFound</m:ResponseCode>"
                    "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                    "</m:DeleteItemResponseMessage>"
                    "<m:DeleteItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:DeleteItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:DeleteItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsDeleteItemRequest> req(new EwsDeleteItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId("9LB1MiL3cOYUjmYy", "TBjl3rnU"),
        EwsId("rZ0sc7Gfn9+XHVgv", "pHTEe9nY")
    };
    req->setItemIds(ids);
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    static const QList<EwsResponseClass> respClasses = {
        EwsResponseError,
        EwsResponseSuccess
    };
    QList<EwsResponseClass>::const_iterator respClassesIt = respClasses.begin();
    unsigned i = 0;
    Q_FOREACH(const EwsDeleteItemRequest::Response &resp, req->responses()) {
        qDebug() << "Verifying response" << i++;
        QCOMPARE(resp.responseClass(), *respClassesIt);
        respClassesIt++;
    }
}

void UtEwsDeleteItemRequest::verifier(FakeTransferJob* job, const QByteArray& req,
                                      const QByteArray &expReq, const QByteArray &response)
{
    bool fail = true;
    auto f = finally([&fail,&job]{
        if (fail) {
            job->postResponse("");
        }
    });
    QCOMPARE(req, expReq);
    fail = false;
    job->postResponse(response);
}

QTEST_MAIN(UtEwsDeleteItemRequest)

#include "ewsdeleteitemrequest_ut.moc"
