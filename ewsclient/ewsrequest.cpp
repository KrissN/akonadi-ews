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

#include "ewsrequest.h"

#include "ewsclient.h"
#include "ewsjobqueue.h"
#include "ewsclient_debug.h"

static const QString ewsReqVersion = QStringLiteral("Exchange2010");

EwsRequest::EwsRequest(EwsClient& client, QObject *parent)
    : EwsJob(parent), mResponseClass(EwsResponseError), mClient(client)
{
}

EwsRequest::~EwsRequest()
{
}

void EwsRequest::doSend()
{
    Q_FOREACH(KJob *job, subjobs()) {
        mClient.mJobQueue->enqueue(job);
    }
}

void EwsRequest::startSoapDocument(QXmlStreamWriter &writer)
{
    writer.setCodec("UTF-8");

    writer.writeStartDocument();

    writer.writeNamespace(soapEnvNsUri, QStringLiteral("soap"));
    writer.writeNamespace(ewsMsgNsUri, QStringLiteral("m"));
    writer.writeNamespace(ewsTypeNsUri, QStringLiteral("t"));

    // SOAP Envelope
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Envelope"));

    // SOAP Header
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Header"));
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("RequestServerVersion"));
    writer.writeAttribute(QStringLiteral("Version"), ewsReqVersion);
    writer.writeEndElement();
    writer.writeEndElement();

    // SOAP Body
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Body"));
}

void EwsRequest::endSoapDocument(QXmlStreamWriter &writer)
{
    // End SOAP Body
    writer.writeEndElement();

    // End SOAP Envelope
    writer.writeEndElement();

    writer.writeEndDocument();
}

void EwsRequest::prepare(const QString body)
{
    KIO::TransferJob *job = KIO::http_post(mClient.url(), body.toUtf8(),
                          KIO::HideProgressInfo);
    job->addMetaData(QStringLiteral("content-type"), QStringLiteral("text/xml"));
    job->addMetaData(mMd);

    connect(job, SIGNAL(result(KJob*)), SLOT(requestResult(KJob*)));
    connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
            SLOT(requestData(KIO::Job*, const QByteArray&)));

    addSubjob(job);
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
        setErrorMsg(QStringLiteral("Failed to process EWS request: ") + job->errorString());
    }
    else {
        QXmlStreamReader reader(mResponseData);
        readResponse(reader);
    }

    emitResult();
}

bool EwsRequest::readResponse(QXmlStreamReader &reader)
{
    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request XML"));
    }

    if ((reader.name() != QStringLiteral("Envelope")) || (reader.namespaceUri() != soapEnvNsUri)) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != soapEnvNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
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
            && (reader.namespaceUri() == soapEnvNsUri)) {
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

    setErrorMsg(faultCode + QStringLiteral(": ") + faultString);

    return false;
}

void EwsRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);

    qCDebug(EWSCLIENT_LOG) << "data" << data;
    mResponseData += QString::fromUtf8(data);
}

bool EwsRequest::parseResponseMessage(QXmlStreamReader &reader, QString reqName,
                          std::function<bool(QXmlStreamReader &reader)> contentReader)
{
    if (reader.name() != reqName + QStringLiteral("Response")
        || reader.namespaceUri() != ewsMsgNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.")
                        .arg(reqName + QStringLiteral("Response")));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.")
                        .arg(reqName + QStringLiteral("Response")));
    }

    if (reader.name() != QStringLiteral("ResponseMessages")
        || reader.namespaceUri() != ewsMsgNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.")
                        .arg(QStringLiteral("ResponseMessages")));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.")
                        .arg(QStringLiteral("ResponseMessages")));
    }

    if (reader.name() != reqName + QStringLiteral("ResponseMessage")
        || reader.namespaceUri() != ewsMsgNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.")
                        .arg(reqName + QStringLiteral("ResponseMessage")));
    }

    if (!readResponseAttr(reader.attributes()))
        return false;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Unexpected namespace in %1 element:")
                            .arg(QStringLiteral("ResponseMessage"))
                            << reader.namespaceUri();
            return false;
        }

        if (!readResponseElement(reader)) {
            if (!contentReader(reader))
                return false;
        }
    }

    return true;
}

bool EwsRequest::readResponseAttr(const QXmlStreamAttributes &attrs)
{
    static const QString respClasses[] = {
        QStringLiteral("Success"),
        QStringLiteral("Warning"),
        QStringLiteral("Error")
    };

    QStringRef respClassRef = attrs.value(QStringLiteral("ResponseClass"));
    if (respClassRef.isNull()) {
        qCWarning(EWSCLIENT_LOG) << "ResponseClass attribute not found in response element";
        return false;
    }

    unsigned i;
    for (i = 0; i < sizeof(respClasses) / sizeof(respClasses[0]); i++) {
        if (respClassRef == respClasses[i]) {
            mResponseClass = static_cast<EwsResponseClass>(i);
            break;
        }
    }

    return (i < sizeof(respClasses) / sizeof(respClasses[0]));
}

bool EwsRequest::readResponseElement(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri) {
        return false;
    }
    if (reader.name() == QStringLiteral("ResponseCode")) {
        mResponseCode = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("MessageText")) {
        mResponseMessage = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("DescriptiveLinkKey")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("MessageXml")) {
        reader.skipCurrentElement();
    }
    else {
        return false;
    }
    return true;
}

