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

#ifndef FAKETRANSFERJOB_H
#define FAKETRANSFERJOB_H

#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <KCoreAddons/KJob>
#include <KIO/SpecialJob>

#include <functional>

namespace KIO {
class Job;
}

template <typename F>
struct Finally {
    Finally(F f): cleanupf{f} {};
    ~Finally() { cleanupf(); };
    F cleanupf;
};

template <typename F>
Finally<F> finally(F f) { return Finally<F>(f); }

class FakeTransferJob : public KIO::SpecialJob
{
    Q_OBJECT
public:
    typedef std::function<void(FakeTransferJob*, const QByteArray &)> VerifierFn;

    struct Verifier {
        QObject *object;
        VerifierFn fn;
    };

    FakeTransferJob(const QByteArray &postData, VerifierFn fn, QObject *parent = 0);
    ~FakeTransferJob();

    static void addVerifier(QObject *obj, VerifierFn fn);
    static Verifier getVerifier();
public Q_SLOTS:
    void postResponse(const QByteArray &resp);
private Q_SLOTS:
    void callVerifier();
    void doEmitResult();
Q_SIGNALS:
    void data(KIO::Job *job, const QByteArray &data);
    void requestReceived(FakeTransferJob *job, const QByteArray &req);
private:
    QByteArray mPostData;
    QByteArray mResponse;
    VerifierFn mVerifier;
    static QQueue<Verifier> mVerifierQueue;
};

#endif
