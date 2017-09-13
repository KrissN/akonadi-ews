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

#ifndef EWSUNSUBSCRIBEREQUEST_H
#define EWSUNSUBSCRIBEREQUEST_H

#include <QSharedPointer>

#include "ewsid.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class EwsUnsubscribeRequest : public EwsRequest
{
    Q_OBJECT
public:

    enum Type {
        PullSubscription = 0,
        PushSubscription,
        StreamingSubscription
    };

    class Response : public EwsRequest::Response
    {
    protected:
        Response(QXmlStreamReader &reader);

        friend class EwsUnsubscribeRequest;
    };

    EwsUnsubscribeRequest(EwsClient &client, QObject *parent);
    virtual ~EwsUnsubscribeRequest();

    void setSubscriptionId(const QString &id)
    {
        mSubscriptionId = id;
    };

    const Response &response() const
    {
        return *mResponse;
    };

    virtual void start() override;
protected:
    virtual bool parseResult(QXmlStreamReader &reader) override;
    bool parseUnsubscribeResponse(QXmlStreamReader &reader);
private:
    QString mSubscriptionId;

    QSharedPointer<Response> mResponse;
};

#endif
