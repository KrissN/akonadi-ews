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

#ifndef EWSREQUEST_H
#define EWSREQUEST_H

#include <functional>

#include <QPointer>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <KIO/TransferJob>

#include "ewsclient.h"
#include "ewsjob.h"
#include "ewsserverversion.h"
#include "ewstypes.h"

class EwsRequest : public EwsJob
{
    Q_OBJECT
public:
    class Response
    {
    public:
        EwsResponseClass responseClass() const
        {
            return mClass;
        };
        bool isSuccess() const
        {
            return mClass == EwsResponseSuccess;
        };
        QString responseCode() const
        {
            return mCode;
        };
        QString responseMessage() const
        {
            return mMessage;
        };
    protected:
        Response(QXmlStreamReader &reader);
        bool readResponseElement(QXmlStreamReader &reader);
        bool setErrorMsg(const QString &msg);

        EwsResponseClass mClass;
        QString mCode;
        QString mMessage;
    };

    EwsRequest(EwsClient &client, QObject *parent);
    virtual ~EwsRequest();

    void setMetaData(const KIO::MetaData &md);
    void addMetaData(const QString &key, const QString &value);

    void setServerVersion(const EwsServerVersion &version);
    const EwsServerVersion &serverVersion() const
    {
        return mServerVersion;
    };

    void dump() const;

protected:
    typedef std::function<bool(QXmlStreamReader &reader)> ContentReaderFn;

    void doSend();
    void prepare(const QString &body);
    virtual bool parseResult(QXmlStreamReader &reader) = 0;
    void startSoapDocument(QXmlStreamWriter &writer);
    void endSoapDocument(QXmlStreamWriter &writer);
    bool parseResponseMessage(QXmlStreamReader &reader, const QString &reqName,
                              ContentReaderFn contentReader);
    bool readResponse(QXmlStreamReader &reader);

    KIO::MetaData mMd;
    QString mResponseData;
protected Q_SLOTS:
    void requestResult(KJob *job);
    void requestData(KIO::Job *job, const QByteArray &data);
private:
    bool readSoapBody(QXmlStreamReader &reader);
    bool readSoapFault(QXmlStreamReader &reader);
    bool readHeader(QXmlStreamReader &reader);
    bool readResponseAttr(const QXmlStreamAttributes &attrs, EwsResponseClass &responseClass);

    QString mBody;
    EwsClient &mClient;
    EwsServerVersion mServerVersion;
};

#endif
