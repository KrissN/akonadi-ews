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

#ifndef EWSREQUEST_H
#define EWSREQUEST_H

#include <functional>

#include <QtCore/QPointer>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include <KIO/TransferJob>

#include "ewsclient.h"
#include "ewsjob.h"
#include "ewstypes.h"

class EwsRequest : public EwsJob
{
    Q_OBJECT
public:
    EwsRequest(EwsClient& client, QObject *parent);
    virtual ~EwsRequest();

    void setMetaData(const KIO::MetaData &md);
    void addMetaData(QString key, QString value);

protected:
    void doSend();
    void prepare(const QString body);
    virtual bool parseResult(QXmlStreamReader &reader) = 0;
    void startSoapDocument(QXmlStreamWriter &writer);
    void endSoapDocument(QXmlStreamWriter &writer);
    bool parseResponseMessage(QXmlStreamReader &reader, QString reqName,
                              std::function<bool(QXmlStreamReader &reader)> contentReader);

    KIO::MetaData mMd;
    EwsResponseClass mResponseClass;
    QString mResponseCode;
    QString mResponseMessage;
private Q_SLOTS:
    void requestResult(KJob *job);
    void requestData(KIO::Job *job, const QByteArray &data);
private:
    bool readResponse(QXmlStreamReader &reader);
    bool readSoapBody(QXmlStreamReader &reader);
    bool readSoapFault(QXmlStreamReader &reader);
    bool readResponseAttr(const QXmlStreamAttributes &attrs);
    bool readResponseElement(QXmlStreamReader &reader);

    QString mResponseData;
    EwsClient &mClient;
};

#endif
