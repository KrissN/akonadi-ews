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

#ifndef FAKEEWSSERVER_H
#define FAKEEWSSERVER_H

#include <functional>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

class __attribute__((visibility("default"))) FakeEwsServer : public QTcpServer
{
    Q_OBJECT
public:
    class DialogEntry
    {
    public:
        typedef QPair<QString, ushort> HttpResponse;
        typedef std::function<bool (const QString &)> ConditionCallback;
        typedef std::function<HttpResponse (const QString &)> ReplyCallback;
        QString expected;
        QRegularExpression expectedRe;
        HttpResponse response;
        ConditionCallback conditionCallback;
        ReplyCallback replyCallback;

        typedef QVector<DialogEntry> List;
    };

    static Q_CONSTEXPR ushort Port = 13548;

    FakeEwsServer(const DialogEntry::List &dialog, DialogEntry::ReplyCallback defaultReplyCallback,
                  QObject *parent);
    virtual ~FakeEwsServer();
/*Q_SIGNALS:
    void requestReceived(QByteArray req);*/
/*public Q_SLOTS:
    void sendResponse(const QByteArray &resp, bool closeConn = false);
    void sendError(const QLatin1String &msg, ushort code = 500);*/
private Q_SLOTS:
    void newConnectionReceived();
    //void connectionClosed(QObject *obj);
private:
    void dataAvailable(QTcpSocket *sock);
    void sendError(QTcpSocket *sock, const QString &msg, ushort code = 500);
    DialogEntry::HttpResponse parseRequest(const QString &content);

    const DialogEntry::List &mDialog;
    DialogEntry::ReplyCallback mDefaultReplyCallback;
};

#endif
