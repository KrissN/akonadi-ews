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

#include "ewsmoveitemrequest.h"

class UtEwsMoveItemRequest : public QObject
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

void UtEwsMoveItemRequest::singleItem()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:MoveItem><m:ToFolderId>"
                    "<t:FolderId Id=\"R70cDGNT1SqOk2pn\" ChangeKey=\"1DjfJ3dT\"/>"
                    "</m:ToFolderId>"
                    "<m:ItemIds><t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"rqs77HkG\"/></m:ItemIds>"
                    "</m:MoveItem></soap:Body></soap:Envelope>\n";
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
                    "<m:MoveItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "<m:Items>"
                    "<t:Message><t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"JoFvRwDP\"/>"
                    "</t:Message>"
                    "</m:Items>"
                    "</m:MoveItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:MoveItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsMoveItemRequest> req(new EwsMoveItemRequest(mClient, this));
    EwsId::List ids;
    ids << EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("rqs77HkG"));
    req->setItemIds(ids);
    req->setDestinationFolderId(EwsId(QStringLiteral("R70cDGNT1SqOk2pn"), QStringLiteral("1DjfJ3dT")));
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 1);
    EwsMoveItemRequest::Response resp = req->responses().first();
    QCOMPARE(resp.responseClass(), EwsResponseSuccess);
    QCOMPARE(resp.itemId(), EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("JoFvRwDP")));
}

void UtEwsMoveItemRequest::twoItems()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:MoveItem><m:ToFolderId>"
                    "<t:FolderId Id=\"R70cDGNT1SqOk2pn\" ChangeKey=\"1DjfJ3dT\"/>"
                    "</m:ToFolderId>"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"rqs77HkG\"/>"
                    "<t:ItemId Id=\"ntTNOncESwiyAXog\" ChangeKey=\"EDHu5rwK\"/>"
                    "</m:ItemIds>"
                    "</m:MoveItem></soap:Body></soap:Envelope>\n";
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
                    "<m:MoveItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "<m:Items>"
                    "<t:Message><t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"JoFvRwDP\"/>"
                    "</t:Message>"
                    "</m:Items>"
                    "</m:MoveItemResponseMessage>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "<m:Items>"
                    "<t:Message><t:ItemId Id=\"ntTNOncESwiyAXog\" ChangeKey=\"4qbAwd3y\"/>"
                    "</t:Message>"
                    "</m:Items>"
                    "</m:MoveItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:MoveItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsMoveItemRequest> req(new EwsMoveItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("rqs77HkG")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("EDHu5rwK"))
    };
    req->setItemIds(ids);
    req->setDestinationFolderId(EwsId(QStringLiteral("R70cDGNT1SqOk2pn"), QStringLiteral("1DjfJ3dT")));
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    static const EwsId::List newIds = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("JoFvRwDP")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("4qbAwd3y"))
    };
    EwsId::List::const_iterator newIdsIt = newIds.begin();
    Q_FOREACH(const EwsMoveItemRequest::Response &resp, req->responses()) {
        QCOMPARE(resp.responseClass(), EwsResponseSuccess);
        QCOMPARE(resp.itemId(), *newIdsIt);
        newIdsIt++;
    }
}

void UtEwsMoveItemRequest::twoItemsOneFailed()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:MoveItem><m:ToFolderId>"
                    "<t:FolderId Id=\"R70cDGNT1SqOk2pn\" ChangeKey=\"1DjfJ3dT\"/>"
                    "</m:ToFolderId>"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"rqs77HkG\"/>"
                    "<t:ItemId Id=\"ntTNOncESwiyAXog\" ChangeKey=\"EDHu5rwK\"/>"
                    "</m:ItemIds>"
                    "</m:MoveItem></soap:Body></soap:Envelope>\n";
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
                    "<m:MoveItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "<m:Items>"
                    "<t:Message><t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"JoFvRwDP\"/>"
                    "</t:Message>"
                    "</m:Items>"
                    "</m:MoveItemResponseMessage>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Error\">"
                    "<m:MessageText>The specified object was not found in the store.</m:MessageText>"
                    "<m:ResponseCode>ErrorItemNotFound</m:ResponseCode>"
                    "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                    "<m:Items/>"
                    "</m:MoveItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:MoveItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsMoveItemRequest> req(new EwsMoveItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("rqs77HkG")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("EDHu5rwK"))
    };
    req->setItemIds(ids);
    req->setDestinationFolderId(EwsId(QStringLiteral("R70cDGNT1SqOk2pn"), QStringLiteral("1DjfJ3dT")));
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    static const QList<EwsResponseClass> respClasses = {
        EwsResponseSuccess,
        EwsResponseError
    };
    static const EwsId::List newIds = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("JoFvRwDP")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("EDHu5rwK"))
    };
    EwsId::List::const_iterator newIdsIt = newIds.begin();
    QList<EwsResponseClass>::const_iterator respClassesIt = respClasses.begin();
    unsigned i = 0;
    Q_FOREACH(const EwsMoveItemRequest::Response &resp, req->responses()) {
        qDebug() << "Verifying response" << i++;
        QCOMPARE(resp.responseClass(), *respClassesIt);
        if (resp.isSuccess()) {
            QCOMPARE(resp.itemId(), *newIdsIt);
        }
        else {
            QCOMPARE(resp.itemId(), EwsId());
        }
        newIdsIt++;
        respClassesIt++;
    }
}

void UtEwsMoveItemRequest::twoItemsSecondFailed()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header><t:RequestServerVersion Version=\"Exchange2007_SP1\"/>"
                    "</soap:Header><soap:Body><m:MoveItem><m:ToFolderId>"
                    "<t:FolderId Id=\"R70cDGNT1SqOk2pn\" ChangeKey=\"1DjfJ3dT\"/>"
                    "</m:ToFolderId>"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"Xnn2DwwaXQUhbn7U\" ChangeKey=\"rqs77HkG\"/>"
                    "<t:ItemId Id=\"ntTNOncESwiyAXog\" ChangeKey=\"EDHu5rwK\"/>"
                    "</m:ItemIds>"
                    "</m:MoveItem></soap:Body></soap:Envelope>\n";
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
                    "<m:MoveItemResponse "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Error\">"
                    "<m:MessageText>The specified object was not found in the store.</m:MessageText>"
                    "<m:ResponseCode>ErrorItemNotFound</m:ResponseCode>"
                    "<m:DescriptiveLinkKey>0</m:DescriptiveLinkKey>"
                    "<m:Items/>"
                    "</m:MoveItemResponseMessage>"
                    "<m:MoveItemResponseMessage ResponseClass=\"Success\">"
                    "<m:ResponseCode>NoError</m:ResponseCode>"
                    "<m:Items>"
                    "<t:Message><t:ItemId Id=\"ntTNOncESwiyAXog\" ChangeKey=\"4qbAwd3y\"/>"
                    "</t:Message>"
                    "</m:Items>"
                    "</m:MoveItemResponseMessage>"
                    "</m:ResponseMessages>"
                    "</m:MoveItemResponse>"
                    "</s:Body>"
                    "</s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    QScopedPointer<EwsMoveItemRequest> req(new EwsMoveItemRequest(mClient, this));
    static const EwsId::List ids = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("rqs77HkG")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("EDHu5rwK"))
    };
    req->setItemIds(ids);
    req->setDestinationFolderId(EwsId(QStringLiteral("R70cDGNT1SqOk2pn"), QStringLiteral("1DjfJ3dT")));
    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), 2);
    static const QList<EwsResponseClass> respClasses = {
        EwsResponseError,
        EwsResponseSuccess
    };
    static const EwsId::List newIds = {
        EwsId(QStringLiteral("Xnn2DwwaXQUhbn7U"), QStringLiteral("JoFvRwDP")),
        EwsId(QStringLiteral("ntTNOncESwiyAXog"), QStringLiteral("4qbAwd3y"))
    };
    EwsId::List::const_iterator newIdsIt = newIds.begin();
    QList<EwsResponseClass>::const_iterator respClassesIt = respClasses.begin();
    unsigned i = 0;
    Q_FOREACH(const EwsMoveItemRequest::Response &resp, req->responses()) {
        qDebug() << "Verifying response" << i++;
        QCOMPARE(resp.responseClass(), *respClassesIt);
        if (resp.isSuccess()) {
            QCOMPARE(resp.itemId(), *newIdsIt);
        }
        else {
            QCOMPARE(resp.itemId(), EwsId());
        }
        newIdsIt++;
        respClassesIt++;
    }
}

void UtEwsMoveItemRequest::verifier(FakeTransferJob* job, const QByteArray& req,
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

QTEST_MAIN(UtEwsMoveItemRequest)

#include "ewsmoveitemrequest_ut.moc"
