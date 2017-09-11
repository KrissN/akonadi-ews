/*
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

#include "ewsautodiscoveryjob.h"

#include <KI18n/KLocalizedString>
#include "ewspoxautodiscoverrequest.h"
#include "ewsclient_debug.h"

EwsAutodiscoveryJob::EwsAutodiscoveryJob(QString email, QString username, QString password,
                                         const QString &userAgent, bool enableNTLMv2, QObject *parent)
    : EwsJob(parent), mEmail(email), mUsername(username), mPassword(password), mUserAgent(userAgent),
      mEnableNTLMv2(enableNTLMv2), mUsedCreds(false)
{
}

EwsAutodiscoveryJob::~EwsAutodiscoveryJob()
{
}

void EwsAutodiscoveryJob::start()
{
    parseEmail();

    if (!mUrlQueue.isEmpty()) {
        sendNextRequest(false);
    }
}

void EwsAutodiscoveryJob::parseEmail()
{
    int atIndex = mEmail.indexOf(QChar::fromLatin1('@'));
    if (atIndex < 0) {
        setErrorMsg(i18n("Incorrect email address"));
        emitResult();
        return;
    }

    QString domain = mEmail.mid(atIndex + 1);
    if (domain.isEmpty()) {
        setErrorMsg(i18n("Incorrect email address"));
        emitResult();
        return;
    }

    addUrls(domain);
}

void EwsAutodiscoveryJob::addUrls(const QString &domain)
{
    mUrlQueue.enqueue(QStringLiteral("https://") + domain + QStringLiteral("/autodiscover/autodiscover.xml"));
    mUrlQueue.enqueue(QStringLiteral("https://autodiscover.") + domain + QStringLiteral("/autodiscover/autodiscover.xml"));
    mUrlQueue.enqueue(QStringLiteral("http://") + domain + QStringLiteral("/autodiscover/autodiscover.xml"));
    mUrlQueue.enqueue(QStringLiteral("http://autodiscover.") + domain + QStringLiteral("/autodiscover/autodiscover.xml"));
}

void EwsAutodiscoveryJob::sendNextRequest(bool useCreds)
{
    QUrl url(mUrlQueue.head());
    if (useCreds) {
        url.setUserName(mUsername);
        url.setPassword(mPassword);
    }
    mUsedCreds = useCreds;
    EwsPoxAutodiscoverRequest *req = new EwsPoxAutodiscoverRequest(url, mEmail, mUserAgent,
                                                                   mEnableNTLMv2, this);
    connect(req, &EwsPoxAutodiscoverRequest::result, this,
            &EwsAutodiscoveryJob::autodiscoveryRequestFinished);
    req->start();
}

void EwsAutodiscoveryJob::autodiscoveryRequestFinished(KJob *job)
{
    qDebug() << "autodiscoveryRequestFinished";
    EwsPoxAutodiscoverRequest *req = qobject_cast<EwsPoxAutodiscoverRequest*>(job);
    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsPoxAutodiscoverRequest job object"));
        emitResult();
    }

    if (req->error()) {

        if (req->error() == 401 && !mUsedCreds &&
            req->lastHttpUrl().scheme() != QStringLiteral("http")) { // Don't try authentication over HTTP

            /* The 401 error may have come from an URL different to the original one (due to
             * redirections). When the original URL is retried with credentials KIO HTTP will issue
             * a warning that an authenticated request is made to a host that never asked for it.
             * To fix this restart the request using the last URL that resulted in the 401 code. */
            mUrlQueue.head() = req->lastHttpUrl().toString();
            sendNextRequest(true);
            return;
        }
        else {
            mUrlQueue.removeFirst();
        }

        if (mUrlQueue.isEmpty()) {
            setErrorText(job->errorText());
            setError(job->error());
            emitResult();
        }
        else {
            sendNextRequest(false);
        }
    }
    else {
        switch (req->action()) {
        case EwsPoxAutodiscoverRequest::Settings:
        {
            EwsPoxAutodiscoverRequest::Protocol proto = req->protocol(EwsPoxAutodiscoverRequest::ExchangeProto);
            if (!proto.isValid()) {
                setErrorMsg(i18n("Exchange protocol information not found"));
            }
            else {
                mEwsUrl = proto.ewsUrl();
                mOabUrl = proto.oabUrl();
            }
            emitResult();
            break;
        }
        case EwsPoxAutodiscoverRequest::RedirectAddr:
            qDebug() << req->redirectAddr();
            mEmail = req->redirectAddr();
            mUrlQueue.clear();
            parseEmail();
            if (!mUrlQueue.isEmpty()) {
                sendNextRequest(false);
            }
            break;
        case EwsPoxAutodiscoverRequest::RedirectUrl:
            qDebug() << req->redirectUrl();
            mUrlQueue.clear();
            mUrlQueue.enqueue(req->redirectUrl());
            sendNextRequest(false);
            break;
        }
    }
}
