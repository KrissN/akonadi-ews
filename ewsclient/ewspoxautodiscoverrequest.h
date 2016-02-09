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

#ifndef EWSPOXAUTODISCOVERREQUEST_H
#define EWSPOXAUTODISCOVERREQUEST_H

#include <QtCore/QString>
#include <QtCore/QUrl>

#include "ewsjob.h"
#include "ewstypes.h"
#include "ewsserverversion.h"

namespace KIO {
class Job;
}

class EwsPoxAutodiscoverRequest : public EwsJob
{
    Q_OBJECT
public:
    enum Action {
        RedirectUrl = 0,
        RedirectAddr,
        Settings,
    };

    enum ProtocolType {
        ExchangeProto,
        ExchangeProxyProto,
        ExchangeWebProto,
        UnknownProto
    };

    class Protocol {
    public:
        Protocol() : mType(UnknownProto) {};
        bool isValid() const { return mType != UnknownProto; };
        ProtocolType type() const { return mType; };
        const QString &ewsUrl() const { return mEwsUrl; };
        const QString &oabUrl() const { return mOabUrl; };
    private:
        ProtocolType mType;
        QString mEwsUrl;
        QString mOabUrl;
        friend class EwsPoxAutodiscoverRequest;
    };

    EwsPoxAutodiscoverRequest(const QUrl &url, const QString &email, QObject *parent);
    virtual ~EwsPoxAutodiscoverRequest();

    const EwsServerVersion &serverVersion() const { return mServerVersion; };

    void dump() const;

    virtual void start() Q_DECL_OVERRIDE;

    Action action() const { return mAction; };
    const Protocol protocol(ProtocolType type) const { return mProtocols.value(type); };
    const QString &redirectAddr() const { return mRedirectAddr; };
    const QString &redirectUrl() const { return mRedirectUrl; };

protected:
    void doSend();
    void prepare(const QString body);
    bool readResponse(QXmlStreamReader &reader);

protected Q_SLOTS:
    void requestResult(KJob *job);
    void requestData(KIO::Job *job, const QByteArray &data);
private:
    bool readAccount(QXmlStreamReader &reader);
    bool readProtocol(QXmlStreamReader &reader);

    QString mResponseData;
    QString mBody;
    QUrl mUrl;
    QString mEmail;
    EwsServerVersion mServerVersion;
    Action mAction;
    QString mRedirectUrl;
    QString mRedirectAddr;
    QHash<ProtocolType, Protocol> mProtocols;
};

#endif
