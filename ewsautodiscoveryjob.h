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

#ifndef EWSAUTODISCOVERYJOB_H
#define EWSAUTODISCOVERYJOB_H

#include <QtCore/QQueue>

#include "ewsjob.h"

class EwsAutodiscoveryJob : public EwsJob
{
    Q_OBJECT
public:
    EwsAutodiscoveryJob(QString email, QString username, QString password, const QString &userAgent,
                        QObject *parent);
    virtual ~EwsAutodiscoveryJob();

    virtual void start() Q_DECL_OVERRIDE;

    const QString &ewsUrl() const { return mEwsUrl; };
    const QString &oabUrl() const { return mOabUrl; };

private Q_SLOTS:
    void autodiscoveryRequestFinished(KJob *job);
private:
    void addUrls(const QString &domain);
    void sendNextRequest(bool useCreds);
    void parseEmail();

    QString mEmail;
    QString mUsername;
    QString mPassword;

    QString mUserAgent;

    QQueue<QString> mUrlQueue;

    QString mEwsUrl;
    QString mOabUrl;
    bool mUsedCreds;
};

#endif
