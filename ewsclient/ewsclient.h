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

#ifndef EWSCLIENT_H
#define EWSCLIENT_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QPointer>
#include <QtCore/QUrl>

#include "ewsserverversion.h"

class EwsClient : public QObject
{
    Q_OBJECT
public:
    EwsClient(QObject *parent = 0);
    ~EwsClient();

    void setUrl(QString url)
    {
        mUrl.setUrl(url);
    }

    void setCredentials(QString username, QString password)
    {
        mUrl.setUserName(username);
        mUrl.setPassword(password);
    }

    enum RequestedConfiguration 
    {
        MailTips = 0,
        UnifiedMessagingConfiguration,
        ProtectionRules
    };

    QUrl url() const
    {
        return mUrl;
    }

    bool isConfigured() const
    {
        return !mUrl.isEmpty();
    }

    void setServerVersion(const EwsServerVersion &version);
    const EwsServerVersion &serverVersion() const { return mServerVersion; };

    void setUserAgent(const QString &userAgent) { mUserAgent = userAgent; };
    const QString &userAgent() const { return mUserAgent; };

    static QHash<QString, QString> folderHash;
private:
    QUrl mUrl;
    QString mUsername;
    QString mPassword;

    QString mUserAgent;

    EwsServerVersion mServerVersion;

    friend class EwsRequest;
};

#endif
