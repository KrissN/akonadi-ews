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

#include "ewsjobqueue.h"

#include "ewsclient_debug.h"

static const int maxConrurrentJobs = 1;

EwsJobQueue::EwsJobQueue(QObject *parent)
    : QObject(parent)
{
}

EwsJobQueue::~EwsJobQueue()
{
}

void EwsJobQueue::enqueue(KJob *job)
{
    qCDebug(EWSCLIENT_LOG) << "Adding job" << job << "to queue.";
    mJobQueue.enqueue(job);
    connect(job, SIGNAL(finished(KJob*)), SLOT(jobFinished(KJob*)));
    maybeStartNextJob();
}

void EwsJobQueue::jobFinished(KJob *job)
{
    qCDebug(EWSCLIENT_LOG) << "Job" << job << "finished.";
    mActiveJobs--;
    maybeStartNextJob();
}

void EwsJobQueue::maybeStartNextJob()
{
    QMutexLocker l(&mJobStartMutex);
    qCDebug(EWSCLIENT_LOG) << mActiveJobs << "active job(s).";
    if ((mActiveJobs < maxConrurrentJobs) && (!mJobQueue.isEmpty())) {
        mActiveJobs++;
        KJob *job = mJobQueue.dequeue();
        qCDebug(EWSCLIENT_LOG) << "Starting job" << job;
        job->start();
    }
}
