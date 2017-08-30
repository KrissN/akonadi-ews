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

#include "ewsgetstreamingeventsrequest.h"

#include <QtCore/QTemporaryFile>

#include "ewsxml.h"
#include "ewsclient_debug.h"

static Q_CONSTEXPR uint respChunkTimeout = 250; /* ms */

EwsGetStreamingEventsRequest::EwsGetStreamingEventsRequest(EwsClient &client, QObject *parent)
    : EwsEventRequestBase(client, QStringLiteral("GetStreamingEvents"), parent), mTimeout(30),
      mRespTimer(this)
{
    mRespTimer.setInterval(respChunkTimeout);
    connect(&mRespTimer, &QTimer::timeout, this, &EwsGetStreamingEventsRequest::requestDataTimeout);
}

EwsGetStreamingEventsRequest::~EwsGetStreamingEventsRequest()
{
}

void EwsGetStreamingEventsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    if (!serverVersion().supports(EwsServerVersion::StreamingSubscription)) {
        setServerVersion(EwsServerVersion::minSupporting(EwsServerVersion::StreamingSubscription));
    }

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("GetStreamingEvents"));

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SubscriptionIds"));
    writer.writeTextElement(ewsTypeNsUri, QStringLiteral("SubscriptionId"), mSubscriptionId);
    writer.writeEndElement();

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("ConnectionTimeout"), QString::number(mTimeout));

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSRES_REQUEST_LOG) << QStringLiteral("Starting GetStreamingEvents request (subId: %1, timeout: %2)")
                    .arg(ewsHash(mSubscriptionId)).arg(mTimeout);

    qCDebug(EWSRES_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

void EwsGetStreamingEventsRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);

    mRespTimer.stop();
    qCDebug(EWSRES_PROTO_LOG) << "data" << job << data;
    mResponseData += QString::fromUtf8(data);
    mRespTimer.start();
}

void EwsGetStreamingEventsRequest::requestDataTimeout()
{
    if (mResponseData.isEmpty()) {
        return;
    }
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

    QXmlStreamReader reader(mResponseData);
    if (!readResponse(reader)) {
        Q_FOREACH(KJob *job, subjobs()) {
            removeSubjob(job);
            job->kill();
        }
        emitResult();
    }
    else {
        Q_EMIT eventsReceived(this);
    }

    mResponseData.clear();
}

void EwsGetStreamingEventsRequest::eventsProcessed(const Response &resp)
{
    mResponses.removeOne(resp);
}
