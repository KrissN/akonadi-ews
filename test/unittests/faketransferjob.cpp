/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include "faketransferjob.h"

#include <QDebug>
#include <QUrl>

#include <KIO/TransferJob>

QQueue<FakeTransferJob::Verifier> FakeTransferJob::mVerifierQueue;

FakeTransferJob::FakeTransferJob(const QByteArray &postData, VerifierFn fn, QObject *parent)
    : KIO::SpecialJob(QUrl(QStringLiteral("file:///tmp/")), QByteArray()), mPostData(postData),
      mVerifier(fn)
{
    metaObject()->invokeMethod(this, "callVerifier", Qt::QueuedConnection);
}

FakeTransferJob::~FakeTransferJob()
{

}

void FakeTransferJob::callVerifier()
{
    mVerifier(this, mPostData);
}

void FakeTransferJob::postResponse(const QByteArray &resp)
{
    mResponse = resp;
    qRegisterMetaType<KIO::Job*>();
    metaObject()->invokeMethod(this, "data", Qt::QueuedConnection, Q_ARG(KIO::Job*, this),
        Q_ARG(const QByteArray&, mResponse));
    metaObject()->invokeMethod(this, "doEmitResult", Qt::QueuedConnection);
}

void FakeTransferJob::doEmitResult()
{
    emitResult();
}

void FakeTransferJob::addVerifier(QObject *obj, VerifierFn fn)
{
    Verifier vfy = {obj, fn};
    mVerifierQueue.enqueue(vfy);
}

FakeTransferJob::Verifier FakeTransferJob::getVerifier()
{
    return mVerifierQueue.dequeue();
}
