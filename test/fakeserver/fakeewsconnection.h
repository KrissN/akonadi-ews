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

#ifndef FAKEEWSCONNECTION_H
#define FAKEEWSCONNECTION_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include "fakeewsserver.h"

class QTcpSocket;

class FakeEwsConnection : public QObject
{
    Q_OBJECT
public:

    FakeEwsConnection(QTcpSocket *sock, const FakeEwsServer::DialogEntry::ReplyCallback replyCallback,
                      QObject *parent);
    virtual ~FakeEwsConnection();
/*public Q_SLOTS:
    void sendResponse(const QByteArray &resp, bool closeConn);*/
private Q_SLOTS:
    void disconnected();
    void dataAvailable();
    void dataTimeout();
private:
    void sendError(const QString &msg, ushort code = 500);

    QPointer<QTcpSocket> mSock;
    const FakeEwsServer::DialogEntry::ReplyCallback mReplyCallback;
    uint mContentLength;
    QByteArray mContent;
    QTimer mDataTimer;
};

#endif
