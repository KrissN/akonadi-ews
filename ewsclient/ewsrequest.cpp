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

#include "ewsrequest.h"

#include <QTemporaryFile>

#include "ewsclient.h"
#include "ewsclient_debug.h"
#include "ewsserverversion.h"

EwsRequest::EwsRequest(EwsClient& client, QObject *parent)
    : EwsJob(parent), mClient(client), mServerVersion(EwsServerVersion::ewsVersion2007Sp1)
{
}

EwsRequest::~EwsRequest()
{
}

void EwsRequest::doSend()
{
    Q_FOREACH(KJob *job, subjobs()) {
        job->start();
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
    mServerVersion.writeRequestServerVersion(writer);
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
    mBody = body;
    KIO::TransferJob *job = KIO::http_post(mClient.url(), body.toUtf8(),
                          KIO::HideProgressInfo);
    job->addMetaData(QStringLiteral("content-type"), QStringLiteral("text/xml"));
    job->addMetaData(QStringLiteral("no-auth-prompt"), QStringLiteral("true"));
    if (mClient.isNTLMv2Enabled()) {
        job->addMetaData(QStringLiteral("EnableNTLMv2Auth"), QStringLiteral("true"));
    }
    if (!mClient.userAgent().isEmpty()) {
        job->addMetaData(QStringLiteral("UserAgent"), mClient.userAgent());
    }
    job->addMetaData(mMd);

    connect(job, &KIO::TransferJob::result, this, &EwsRequest::requestResult);
    connect(job, &KIO::TransferJob::data, this, &EwsRequest::requestData);

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
    if (EWSRES_PROTO_LOG().isDebugEnabled()) {
        ewsLogDir.setAutoRemove(false);
        if (ewsLogDir.isValid()) {
            QTemporaryFile dumpFile(ewsLogDir.path() + "/ews_xmldump_XXXXXXX.xml");
            dumpFile.open();
            dumpFile.setAutoRemove(false);
            dumpFile.write(mResponseData.toUtf8());
            qCDebug(EWSRES_PROTO_LOG) << "response dumped to" << dumpFile.fileName();
            dumpFile.close();
        }
    }

    KIO::TransferJob *trJob = qobject_cast<KIO::TransferJob*>(job);
    int resp = trJob->metaData()["responsecode"].toUInt();

    if (job->error() != 0) {
        setErrorMsg(QStringLiteral("Failed to process EWS request: ") + job->errorString(), job->error());
    }
    /* Don't attempt to parse the response in case of a HTTP error. The only exception is
     * 500 (Bad Request) as in such case the server does provide the usual SOAP response. */
    else if ((resp >= 300) && (resp != 500)) {
        setErrorMsg(QStringLiteral("Failed to process EWS request - HTTP code %1").arg(resp));
        setError(resp);
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
            if (!readHeader(reader)) {
                return false;
            }
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

        if (!parseResult(reader)) {
            if (EWSRES_FAILEDREQUEST_LOG().isDebugEnabled()) {
                dump();
            }
            return false;
        }
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

    if (EWSRES_FAILEDREQUEST_LOG().isDebugEnabled()) {
        dump();
    }

    return false;
}

void EwsRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);

    qCDebug(EWSRES_PROTO_LOG) << "data" << job << data;
    mResponseData += QString::fromUtf8(data);
}

bool EwsRequest::parseResponseMessage(QXmlStreamReader &reader, QString reqName,
                          ContentReaderFn contentReader)
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

    while (reader.readNextStartElement()) {
        if (reader.name() != reqName + QStringLiteral("ResponseMessage")
            || reader.namespaceUri() != ewsMsgNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.")
                            .arg(reqName + QStringLiteral("ResponseMessage")));
        }

        if (!contentReader(reader)) {
            return false;
        }
    }

    return true;
}

void EwsRequest::setServerVersion(const EwsServerVersion &version)
{
    mServerVersion = version;
}

EwsRequest::Response::Response(QXmlStreamReader &reader)
{
    static const QString respClasses[] = {
        QStringLiteral("Success"),
        QStringLiteral("Warning"),
        QStringLiteral("Error")
    };

    QStringRef respClassRef = reader.attributes().value(QStringLiteral("ResponseClass"));
    if (respClassRef.isNull()) {
        mClass = EwsResponseParseError;
        qCWarning(EWSRES_LOG) << "ResponseClass attribute not found in response element";
        return;
    }

    unsigned i;
    for (i = 0; i < sizeof(respClasses) / sizeof(respClasses[0]); ++i) {
        if (respClassRef == respClasses[i]) {
            mClass = static_cast<EwsResponseClass>(i);
            break;
        }
    }
}

bool EwsRequest::Response::readResponseElement(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri) {
        return false;
    }
    if (reader.name() == QStringLiteral("ResponseCode")) {
        mCode = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("MessageText")) {
        mMessage = reader.readElementText();
    }
    else if (reader.name() == QStringLiteral("DescriptiveLinkKey")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("MessageXml")) {
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ErrorSubscriptionIds")) {
        reader.skipCurrentElement();
    }
    else {
        return false;
    }
    return true;
}

bool EwsRequest::readHeader(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("ServerVersionInfo") && reader.namespaceUri() == ewsTypeNsUri) {
            EwsServerVersion version(reader);
            if (!version.isValid()) {
                qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read EWS request - error parsing server version.");
                return false;
            }
            mServerVersion = version;
            mClient.setServerVersion(version);
            reader.skipCurrentElement();
        }
        else {
            reader.skipCurrentElement();
        }
    }

    return true;
}

bool EwsRequest::Response::setErrorMsg(const QString msg)
{
    mClass = EwsResponseParseError;
    mCode = QStringLiteral("ResponseParseError");
    mMessage = msg;
    qCWarningNC(EWSRES_LOG) << msg;
    return false;
}

void EwsRequest::dump() const
{
    ewsLogDir.setAutoRemove(false);
    if (ewsLogDir.isValid()) {
        QTemporaryFile reqDumpFile(ewsLogDir.path() + "/ews_xmlreqdump_XXXXXXX.xml");
        reqDumpFile.open();
        reqDumpFile.setAutoRemove(false);
        reqDumpFile.write(mBody.toUtf8());
        reqDumpFile.close();
        QTemporaryFile resDumpFile(ewsLogDir.path() + "/ews_xmlresdump_XXXXXXX.xml");
        resDumpFile.open();
        resDumpFile.setAutoRemove(false);
        resDumpFile.write(mResponseData.toUtf8());
        resDumpFile.close();
        qCDebug(EWSRES_LOG) << "request  dumped to" << reqDumpFile.fileName();
        qCDebug(EWSRES_LOG) << "response dumped to" << resDumpFile.fileName();
    } else {
        qCWarning(EWSRES_LOG) << "failed to dump request and response";
    }
}
