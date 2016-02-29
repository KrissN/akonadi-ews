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

#include <QtTest/QtTest>
#include <QtCore/QEventLoop>

#include "fakehttppost.h"

#include "ewsgetitemrequest.h"

class UtEwsGetItemRequest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void twoFailures();
private:
    void verifier(FakeTransferJob* job, const QByteArray& req, const QByteArray &expReq,
                  const QByteArray &resp);

    EwsClient mClient;
};

void UtEwsGetItemRequest::twoFailures()
{
    static const QByteArray request = "<?xml version=\"1.0\"?>"
                    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
                    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<soap:Header>"
                    "<t:RequestServerVersion Version=\"Exchange2007_SP1\"/></soap:Header>"
                    "<soap:Body>"
                    "<m:GetItem>"
                    "<m:ItemShape><t:BaseShape>IdOnly</t:BaseShape></m:ItemShape>"
                    "<m:ItemIds>"
                    "<t:ItemId Id=\"DdBTBAvLHI8OyQ3K\" ChangeKey=\"6yDDqXl+\"/>"
                    "<t:ItemId Id=\"CgIdfZGT3QJrWZHi\" ChangeKey=\"wPjRsOpg\"/>"
                    "<t:ItemId Id=\"Enxw15n4imIERH4w\" ChangeKey=\"82pEQQIj\"/>"
                    "<t:ItemId Id=\"yV1OhxPOinZ7mxpK\" ChangeKey=\"B22tdkME\"/>"
                    "<t:ItemId Id=\"j1ptydBqXKJLuCiB\" ChangeKey=\"z0u+e6/Z\"/>"
                    "<t:ItemId Id=\"ogM0ejAHml/og1tZ\" ChangeKey=\"f2t/ou/g\"/>"
                    "<t:ItemId Id=\"ZqDVkG1gIrUkJDGB\" ChangeKey=\"0LOh2uE+\"/>"
                    "<t:ItemId Id=\"SFYgXaYm1DK+0TCs\" ChangeKey=\"Zbkp+aB4\"/>"
                    "<t:ItemId Id=\"UrNr/v4HynI062u/\" ChangeKey=\"WMjq6rUe\"/>"
                    "</m:ItemIds>"
                    "</m:GetItem>"
                    "</soap:Body>"
                    "</soap:Envelope>\n";
    static const QByteArray response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                    "<s:Header>"
                    "<h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" MajorBuildNumber=\"266\" MinorBuildNumber=\"1\" Version=\"Exchange2010_SP2\" xmlns:h=\"http://schemas.microsoft.com/exchange/services/2006/types\" xmlns=\"http://schemas.microsoft.com/exchange/services/2006/types\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"/>"
                    "</s:Header>"
                    "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
                    "<m:GetItemResponse xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
                    "<m:ResponseMessages>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"DdBTBAvLHI8OyQ3K\" ChangeKey=\"6yDDqXl+\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"CgIdfZGT3QJrWZHi\" ChangeKey=\"wPjRsOpg\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"Enxw15n4imIERH4w\" ChangeKey=\"82pEQQIj\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"yV1OhxPOinZ7mxpK\" ChangeKey=\"B22tdkME\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"j1ptydBqXKJLuCiB\" ChangeKey=\"z0u+e6/Z\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"ogM0ejAHml/og1tZ\" ChangeKey=\"f2t/ou/g\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Success\"><m:ResponseCode>NoError</m:ResponseCode><m:Items><t:Message><t:ItemId Id=\"ZqDVkG1gIrUkJDGB\" ChangeKey=\"0LOh2uE+\"/></t:Message></m:Items></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Error\"><m:MessageText>The specified object was not found in the store.</m:MessageText><m:ResponseCode>ErrorItemNotFound</m:ResponseCode><m:DescriptiveLinkKey>0</m:DescriptiveLinkKey><m:Items/></m:GetItemResponseMessage>"
                    "<m:GetItemResponseMessage ResponseClass=\"Error\"><m:MessageText>The specified object was not found in the store.</m:MessageText><m:ResponseCode>ErrorItemNotFound</m:ResponseCode><m:DescriptiveLinkKey>0</m:DescriptiveLinkKey><m:Items/></m:GetItemResponseMessage>"
                    "</m:ResponseMessages></m:GetItemResponse></s:Body></s:Envelope>";

    FakeTransferJob::addVerifier(this, [this](FakeTransferJob* job, const QByteArray& req){
        verifier(job, req, request, response);
    });
    EwsGetItemRequest *req = new EwsGetItemRequest(mClient, this);
    EwsId::List ids;
    ids << EwsId("DdBTBAvLHI8OyQ3K", "6yDDqXl+");
    ids << EwsId("CgIdfZGT3QJrWZHi", "wPjRsOpg");
    ids << EwsId("Enxw15n4imIERH4w", "82pEQQIj");
    ids << EwsId("yV1OhxPOinZ7mxpK", "B22tdkME");
    ids << EwsId("j1ptydBqXKJLuCiB", "z0u+e6/Z");
    ids << EwsId("ogM0ejAHml/og1tZ", "f2t/ou/g");
    ids << EwsId("ZqDVkG1gIrUkJDGB", "0LOh2uE+");
    ids << EwsId("SFYgXaYm1DK+0TCs", "Zbkp+aB4");
    ids << EwsId("UrNr/v4HynI062u/", "WMjq6rUe");
    req->setItemIds(ids);

    req->setItemShape(EwsShapeIdOnly);

    req->exec();

    QCOMPARE(req->error(), 0);
    QCOMPARE(req->responses().size(), ids.size());

    static const QList<EwsResponseClass> respClasses = {
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseSuccess,
        EwsResponseError,
        EwsResponseError
    };
    QList<EwsResponseClass>::const_iterator respClassesIt = respClasses.begin();
    EwsId::List::const_iterator idsIt = ids.cbegin();
    unsigned i = 0;
    Q_FOREACH(const EwsGetItemRequest::Response &resp, req->responses()) {
        qDebug() << "Verifying response" << i++;
        QCOMPARE(resp.responseClass(), *respClassesIt);
        if (resp.isSuccess()) {
            EwsId id = resp.item()[EwsItemFieldItemId].value<EwsId>();
            QCOMPARE(id, *idsIt);
        }
        idsIt++;
        respClassesIt++;
    }
}

void UtEwsGetItemRequest::verifier(FakeTransferJob* job, const QByteArray& req,
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

QTEST_MAIN(UtEwsGetItemRequest)

#include "ewsgetitemrequest_ut.moc"
