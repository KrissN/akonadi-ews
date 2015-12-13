/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

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

#include <QtCore/QPointer>
#include <KIO/TransferJob>

#include "ewsclient.h"
#include "ewsxmlitems.h"

class EwsRequest : public QObject
{
    Q_OBJECT
public:
    EwsRequest(EwsClient* parent);
    virtual ~EwsRequest();
    virtual void send() = 0;

    void setMetaData(const KIO::MetaData &md);
    void addMetaData(QString key, QString value);

    bool isError() const { return mError; };
    QString errorString() const { return mErrorString; };
signals:
    void finished(EwsRequest *req);
protected:
    void doSend();
    void prepare(const QString body);
    void prepare(const EwsXmlItemBase *item);
    virtual bool parseResult(QXmlStreamReader &reader) = 0;
    bool setError(const QString msg);
    void startSoapDocument(QXmlStreamWriter &writer);
    void endSoapDocument(QXmlStreamWriter &writer);

    KIO::MetaData mMd;
    QPointer<KIO::TransferJob> mJob;    // The job object deletes itself automatically once finished
                                        // Use a smart pointer to make sure we don't try to do it
                                        // again.
    bool mError;
    QString mErrorString;
private slots:
    void requestResult(KJob *job);
    void requestData(KIO::Job *job, const QByteArray &data);
private:
    bool readResponse(QXmlStreamReader &reader);
    bool readSoapBody(QXmlStreamReader &reader);
    bool readSoapFault(QXmlStreamReader &reader);
    QString mResponseData;
};

#endif
