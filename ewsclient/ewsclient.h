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

#ifndef EWSCLIENT_H
#define EWSCLIENT_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QPointer>
#include <QtCore/QUrl>

class EwsJobQueue;
class EwsRequest;

class EwsClient : public QObject
{
    Q_OBJECT
public:
    EwsClient(QString url, QObject *parent = 0);
    ~EwsClient();

    void setUsername(QString username, QString password)
    {
        mUsername = username;
        mPassword = password;
    }

    enum RequestedConfiguration 
    {
        MailTips = 0,
        UnifiedMessagingConfiguration,
        ProtectionRules
    };

    void getServiceConfiguration(QVector<RequestedConfiguration> conf);
    
    QUrl url() const
    {
        return mUrl;
    }
    
private:
    void sendRequest(QString body);
    
    QUrl mUrl;
    QString mUsername;
    QString mPassword;
    
    QPointer<EwsJobQueue> mJobQueue;

    friend class EwsRequest;
};

#endif
