/*
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

#ifndef FAKEHTTPPOST_H
#define FAKEHTTPPOST_H

#include <QDebug>

#include <KIO/TransferJob>

#include "faketransferjob.h"

namespace KIO
{

TransferJob *http_post(const QUrl &url, const QByteArray &postData, JobFlags flags)
{
    Q_UNUSED(url);
    Q_UNUSED(flags);

    FakeTransferJob::Verifier vfy = FakeTransferJob::getVerifier();
    FakeTransferJob *job = new FakeTransferJob(postData, vfy.fn, vfy.object);
    return reinterpret_cast<TransferJob*>(job);
}

}

#endif
