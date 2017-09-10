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

#ifndef EWSSUBSCRIBEREQUEST_H
#define EWSSUBSCRIBEREQUEST_H

#include <QSharedPointer>

#include "ewsid.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamWriter;

class EwsSubscribeRequest : public EwsRequest
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
    public:
        const QString &subscriptionId() const { return mId; };
        const QString &watermark() const { return mWatermark; };
    protected:
        Response(QXmlStreamReader &reader);

        QString mId;
        QString mWatermark;

        friend class EwsSubscribeRequest;
    };

    EwsSubscribeRequest(EwsClient &client, QObject *parent);
    virtual ~EwsSubscribeRequest();

    //void setSubscription(const EwsSubscription& subscription);
    void setType(Type t) { mType = t; };
    void setFolderIds(EwsId::List folders) { mFolderIds = folders; };
    void setAllFolders(bool allFolders) { mAllFolders = allFolders; };
    void setEventTypes(QList<EwsEventType> types) { mEventTypes = types; };
    void setWatermark(const QString &watermark) { mWatermark = watermark; };
    void setTimeout(uint timeout) { mTimeout = timeout; };

    const Response &response() const { return *mResponse; };

    virtual void start() Q_DECL_OVERRIDE;
protected:
    virtual bool parseResult(QXmlStreamReader &reader) Q_DECL_OVERRIDE;
    bool parseSubscribeResponse(QXmlStreamReader &reader);
private:
    //QSharedPointer<EwsSubscription> mSubscription;
    Type mType;
    EwsId::List mFolderIds;
    QList<EwsEventType> mEventTypes;
    bool mAllFolders;
    QString mWatermark;
    uint mTimeout;

    QSharedPointer<Response> mResponse;
};

#endif
