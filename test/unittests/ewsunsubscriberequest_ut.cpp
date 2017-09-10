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

#include "ewsunsubscriberequest.h"

class UtEwsUnsibscribeRequest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void simple();
private:
    void verifier(FakeTransferJob* job, const QByteArray& req, const QByteArray &expReq,
                  const QByteArray &resp);

    EwsClient mClient;
};

void UtEwsUnsibscribeRequest::simple()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/></soap:Header>"
                    "<soap:Body>"
                    "<m:Unsubscribe>"
                    "<m:SubscriptionId>dwzVKTlwXxBZtQRMucP5Mg==</m:SubscriptionId>"
                    "</m:Unsubscribe></soap:Body></soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header><h:ServerVersionInfo xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "MajorVersion=\"15\" MinorVersion=\"1\" MajorBuildNumber=\"409\" "
                    "MinorBuildNumber=\"24\" Version=\"V2016_01_06\"/>"
                    "</s:Header>"
                    "<s:Body>"
                    "<m:UnsubscribeResponse xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:UnsubscribeResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "</m:UnsubscribeResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:UnsubscribeResponse>"
                    "</s:Body></s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsUnsubscribeRequest> req(new EwsUnsubscribeRequest(mClient, this));
    req->setSubscriptionId("dwzVKTlwXxBZtQRMucP5Mg==");

    req->exec();

    QCOMPARE(req->error(), 0);
}

void UtEwsUnsibscribeRequest::verifier(FakeTransferJob* job, const QByteArray& req,
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

QTEST_MAIN(UtEwsUnsibscribeRequest)

#include "ewsunsubscriberequest_ut.moc"
