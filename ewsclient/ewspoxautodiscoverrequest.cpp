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

#include "ewspoxautodiscoverrequest.h"

#include "ewsclient_debug.h"

#include <QtCore/QTemporaryFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <KIO/TransferJob>


static const QString poxAdOuReqNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/autodiscover/outlook/requestschema/2006");
static const QString poxAdRespNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/autodiscover/responseschema/2006");
static const QString poxAdOuRespNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/autodiscover/outlook/responseschema/2006a");

EwsPoxAutodiscoverRequest::EwsPoxAutodiscoverRequest(const QUrl &url, const QString &email, QObject *parent)
    : EwsJob(parent), mUrl(url), mEmail(email), mServerVersion(EwsServerVersion::ewsVersion2007Sp1),
      mAction(Settings)
{
}

EwsPoxAutodiscoverRequest::~EwsPoxAutodiscoverRequest()
{
}

void EwsPoxAutodiscoverRequest::doSend()
{
    Q_FOREACH(KJob *job, subjobs()) {
        job->start();
    }
}

void EwsPoxAutodiscoverRequest::prepare(const QString body)
{
    mBody = body;
    KIO::TransferJob *job = KIO::http_post(mUrl, body.toUtf8(), KIO::HideProgressInfo);
    job->addMetaData(QStringLiteral("content-type"), QStringLiteral("text/xml"));
    job->addMetaData(QStringLiteral("no-auth-prompt"), QStringLiteral("true"));
    //config->readEntry("no-spoof-check", false)

    connect(job, SIGNAL(result(KJob*)), SLOT(requestResult(KJob*)));
    connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
            SLOT(requestData(KIO::Job*, const QByteArray&)));

    addSubjob(job);
}

void EwsPoxAutodiscoverRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    writer.writeStartDocument();

    writer.writeDefaultNamespace(poxAdOuReqNsUri);

    writer.writeStartElement(poxAdOuReqNsUri, QStringLiteral("Autodiscover"));

    writer.writeStartElement(poxAdOuReqNsUri, QStringLiteral("Request"));

    writer.writeTextElement(poxAdOuReqNsUri, QStringLiteral("EMailAddress"), mEmail);
    writer.writeTextElement(poxAdOuReqNsUri, QStringLiteral("AcceptableResponseSchema"), poxAdOuRespNsUri);

    writer.writeEndElement(); // Request

    writer.writeEndElement(); // Autodiscover

    writer.writeEndDocument();

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting POX Autodiscovery request (url: ")
                    << mUrl << QStringLiteral(", email: ") << mEmail;
    prepare(reqString);

    doSend();
}


void EwsPoxAutodiscoverRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);

    qCDebug(EWSRES_PROTO_LOG) << "data" << job << data;
    mResponseData += QString::fromUtf8(data);
}

void EwsPoxAutodiscoverRequest::requestResult(KJob *job)
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
        setErrorMsg(QStringLiteral("Failed to process EWS request: ") + job->errorString());
        setError(job->error());
    }
    else if (resp >= 300) {
        setErrorMsg(QStringLiteral("Failed to process EWS request - HTTP code %1").arg(resp));
        setError(resp);
    }
    else {
        QXmlStreamReader reader(mResponseData);
        readResponse(reader);
    }

    emitResult();
}

bool EwsPoxAutodiscoverRequest::readResponse(QXmlStreamReader &reader)
{
    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read POX response XML"));
    }

    if ((reader.name() != QStringLiteral("Autodiscover")) || (reader.namespaceUri() != poxAdRespNsUri)) {
        return setErrorMsg(QStringLiteral("Failed to read POX response - not an Autodiscover response"));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read POX response - expected %1 element")
            .arg(QStringLiteral("Response")));
    }

    if ((reader.name() != QStringLiteral("Response")) || (reader.namespaceUri() != poxAdOuRespNsUri)) {
        return setErrorMsg(QStringLiteral("Failed to read POX response - expected %1 element, found %2")
            .arg(QStringLiteral("Response").arg(reader.name().toString())));
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }

        if (reader.name() == QStringLiteral("User")) {
            reader.skipCurrentElement();
        }
        else if (reader.name() == QStringLiteral("Account")) {
            if (!readAccount(reader)) {
                return false;
            }
        }
        else {
            return setErrorMsg(QStringLiteral("Failed to read POX response - unknown element '%1' inside '%2'")
                .arg(reader.name().toString()).arg(QStringLiteral("Response")));
        }
    }
    return true;
}

bool EwsPoxAutodiscoverRequest::readAccount(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }

        if (reader.name() == QStringLiteral("Action")) {
            QString action = reader.readElementText();
            if (action == QStringLiteral("settings")) {
                mAction = Settings;
            }
            else if (action == QStringLiteral("redirectUrl")) {
                mAction = RedirectUrl;
            }
            else if (action == QStringLiteral("redirectAddr")) {
                mAction = RedirectAddr;
            }
            else {
                return setErrorMsg(QStringLiteral("Failed to read POX response - unknown action '%1'")
                    .arg(action));
            }
        }
        else if (reader.name() == QStringLiteral("RedirectUrl")) {
            mRedirectUrl = reader.readElementText();
        }
        else if (reader.name() == QStringLiteral("RedirectAddr")) {
            mRedirectAddr = reader.readElementText();
        }
        else if (reader.name() == QStringLiteral("RedirectAddr")) {
            mRedirectAddr = reader.readElementText();
        }
        else if (reader.name() == QStringLiteral("Protocol")) {
            if (!readProtocol(reader)) {
                return false;
            }
        }
        else {
            reader.skipCurrentElement();
        }
    }
    return true;
}

bool EwsPoxAutodiscoverRequest::readProtocol(QXmlStreamReader &reader)
{
    Protocol proto;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }

        if (reader.name() == QStringLiteral("Type")) {
            QString type = reader.readElementText();
            if (type == QStringLiteral("EXCH")) {
                proto.mType = ExchangeProto;
            }
            else if (type== QStringLiteral("EXPR")) {
                proto.mType = ExchangeProxyProto;
            }
            else if (type == QStringLiteral("WEB")) {
                proto.mType = ExchangeWebProto;
            }
            else {
                return setErrorMsg(QStringLiteral("Failed to read POX response - unknown protocol '%1'")
                    .arg(type));
            }
        }
        else if (reader.name() == QStringLiteral("EwsUrl")) {
            proto.mEwsUrl = reader.readElementText();
        }
        else if (reader.name() == QStringLiteral("OabUrl")) {
            proto.mOabUrl = reader.readElementText();
        }
        else {
            reader.skipCurrentElement();
        }
    }

    qDebug() << "Adding proto type" << proto.mType << proto.isValid();
    mProtocols[proto.mType] = proto;

    return true;
}

void EwsPoxAutodiscoverRequest::dump() const
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
