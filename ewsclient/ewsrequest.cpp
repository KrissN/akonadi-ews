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

#include "ewsrequest.h"

#include "ewsclient.h"
#include "ewsjobqueue.h"
#include "ewsclient_debug.h"

static const QString ewsReqVersion = QStringLiteral("Exchange2010");

EwsRequest::EwsRequest(EwsClient *parent)
    : QObject(parent), mJob(0), mError(false)
{
}

EwsRequest::~EwsRequest()
{
    delete mJob;
}

void EwsRequest::doSend()
{
    qobject_cast<EwsClient*>(parent())->mJobQueue->enqueue(mJob);
}

void EwsRequest::prepare(const QString body)
{
    mJob = KIO::http_post(qobject_cast<EwsClient*>(parent())->url(), body.toUtf8(),
                          KIO::HideProgressInfo);
    mJob->addMetaData("content-type", "text/xml");
    mJob->addMetaData(mMd);

    connect(mJob, SIGNAL(result(KJob*)), SLOT(requestResult(KJob*)));
    connect(mJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
            SLOT(requestData(KIO::Job*, const QByteArray&)));

}

void EwsRequest::prepare(const EwsXmlItemBase *item)
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    writer.setCodec("UTF-8");

    writer.writeStartDocument();

    writer.writeNamespace(EwsXmlItemBase::soapEnvNsUri, QStringLiteral("soap"));
    writer.writeNamespace(EwsXmlItemBase::ewsMsgNsUri, QStringLiteral("m"));
    writer.writeNamespace(EwsXmlItemBase::ewsTypeNsUri, QStringLiteral("t"));

    // SOAP Envelope
    writer.writeStartElement(EwsXmlItemBase::soapEnvNsUri, QStringLiteral("Envelope"));

    // SOAP Header
    writer.writeStartElement(EwsXmlItemBase::soapEnvNsUri, QStringLiteral("Header"));
    writer.writeStartElement(EwsXmlItemBase::ewsTypeNsUri, QStringLiteral("RequestServerVersion"));
    writer.writeAttribute(QStringLiteral("Version"), ewsReqVersion);
    writer.writeEndElement();
    writer.writeEndElement();

    // SOAP Body
    writer.writeStartElement(EwsXmlItemBase::soapEnvNsUri, QStringLiteral("Body"));

    // Content
    item->write(writer);

    // End SOAP Body
    writer.writeEndElement();

    // End SOAP Envelope
    writer.writeEndElement();

    writer.writeEndDocument();

    qCDebug(EWSCLIENT_LOG) << reqString;

    prepare(reqString);
}

void EwsRequest::setMetaData(const KIO::MetaData &md)
{
    mMd = md;
}

void EwsRequest::addMetaData(QString key, QString value)
{
    mMd.insert(key, value);
}

void EwsRequest::requestResult(KJob *job)
{
    qCDebug(EWSCLIENT_LOG) << "result" << job->error();

    if (job->error() != 0) {
        setError(QStringLiteral("Failed to process EWS request: ") + job->errorString());
    }
    else {
        QXmlStreamReader reader(mResponseData);
        readResponse(reader);
    }

    emit finished(this);
}

bool EwsRequest::readResponse(QXmlStreamReader &reader)
{
    if (!reader.readNextStartElement()) {
        return setError(QStringLiteral("Failed to read EWS request XML"));
    }

    if ((reader.name() != QStringLiteral("Envelope")) || (reader.namespaceUri() != EwsXmlItemBase::soapEnvNsUri)) {
        return setError(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != EwsXmlItemBase::soapEnvNsUri) {
            return setError(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
        }

        if (reader.name() == QStringLiteral("Body")) {
            if (!readSoapBody(reader))
                return false;
        }
        else if (reader.name() == QStringLiteral("Header")) {
            reader.skipCurrentElement();
        }
    }
    return true;
}

bool EwsRequest::readSoapBody(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if ((reader.name() == QStringLiteral("Fault"))
            && (reader.namespaceUri() == EwsXmlItemBase::soapEnvNsUri)) {
            return readSoapFault(reader);
        }

        if (!parseResult(reader))
            return false;
    }
    return true;
}

bool EwsRequest::readSoapFault(QXmlStreamReader &reader)
{
    QString faultCode;
    QString faultString;
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("faultcode")) {
            faultCode = reader.readElementText();
        }
        else if (reader.name() == QStringLiteral("faultstring")) {
            faultString = reader.readElementText();
        }
    }

    mError = true;
    mErrorString = faultCode + QStringLiteral(": ") + faultString;

    return false;
}

void EwsRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    qCDebug(EWSCLIENT_LOG) << "data" << data;
    mResponseData += QString::fromUtf8(data);
}

bool EwsRequest::setError(const QString msg)
{
    mError = true;
    mErrorString = msg;
    qCWarning(EWSCLIENT_LOG) << mErrorString;
    return false;
}

