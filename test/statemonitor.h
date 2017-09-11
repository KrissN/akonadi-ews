/*
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef TEST_STATEMONITOR_H
#define TEST_STATEMONITOR_H

#include <functional>

#include <QObject>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Monitor>

class StateMonitorBase : public QObject
{
    Q_OBJECT
public:
    explicit StateMonitorBase(QObject *parent) : QObject(parent) {};
    virtual ~StateMonitorBase() = default;
Q_SIGNALS:
    void stateReached();
    void errorOccurred();
};

template <typename T> class CollectionStateMonitor : public StateMonitorBase
{
public:
    typedef std::function<bool(const Akonadi::Collection &col, const T &state)> StateComparisonFunc;
    CollectionStateMonitor(QObject *parent, const QHash<QString, T> &stateHash,
                           const QString &inboxId, const StateComparisonFunc &comparisonFunc);
    ~CollectionStateMonitor() = default;
    Akonadi::Monitor &monitor() { return mMonitor; };
private:
    void stateChanged(const Akonadi::Collection &col);

    Akonadi::Monitor mMonitor;
    QSet<QString> mPending;
    const QHash<QString, T> &mStateHash;
    const StateComparisonFunc &mComparisonFunc;
    const QString &mInboxId;
};

template <typename T>
CollectionStateMonitor<T>::CollectionStateMonitor(QObject *parent, const QHash<QString, T> &stateHash,
                                                  const QString &inboxId, const StateComparisonFunc &comparisonFunc)
    : StateMonitorBase(parent), mMonitor(this), mPending(stateHash.keys().toSet()), mStateHash(stateHash),
      mComparisonFunc(comparisonFunc), mInboxId(inboxId)
{
    connect(&mMonitor, &Akonadi::Monitor::collectionAdded, this,
            [this](const Akonadi::Collection &col, const Akonadi::Collection &) {
        stateChanged(col);
    });
    connect(&mMonitor, QOverload<const Akonadi::Collection&>::of(&Akonadi::Monitor::collectionChanged), this,
            [this](const Akonadi::Collection &col) {
        stateChanged(col);
    });
}

template <typename T>
void CollectionStateMonitor<T>::stateChanged(const Akonadi::Collection &col)
{
    auto remoteId = col.remoteId() == QStringLiteral("INBOX") ? mInboxId : col.remoteId();
    auto state = mStateHash.find(remoteId);
    if (state == mStateHash.end()) {
        qDebug() << "Cannot find state for collection" << remoteId;
        Q_EMIT errorOccurred();
    }
    if (mComparisonFunc(col, *state)) {
        mPending.remove(remoteId);
    } else {
        mPending.insert(remoteId);
    }
    if (mPending.empty()) {
        Q_EMIT stateReached();
    }
}

#endif
