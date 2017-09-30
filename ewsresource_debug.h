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

#ifndef EWSRESOURCE_DEBUG_H
#define EWSRESOURCE_DEBUG_H

#include <QLoggingCategory>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include "ewsclient.h"
#include "ewsclient_debug.h"
#include "ewsres_agentif_debug.h"
#include "ewsres_debug.h"

inline QDebug operator<<(QDebug debug, const Akonadi::Item::List &items)
{
    QDebugStateSaver saver(debug);
    QStringList itemStrs;
    Q_FOREACH (const Akonadi::Item &item, items) {
        itemStrs.append(ewsHash(item.remoteId()));
    }
    debug.nospace().noquote() << "Akonadi::Item::List(" << itemStrs.join(QChar::fromLatin1(',')) << ")";
    return debug.maybeSpace();
}

inline QDebug operator<<(QDebug debug, const Akonadi::Item &item)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "Akonadi::Item(" << ewsHash(item.remoteId()) << ")";
    return debug.maybeSpace();
}

inline QDebug operator<<(QDebug debug, const Akonadi::Collection::List &cols)
{
    QDebugStateSaver saver(debug);
    QStringList itemStrs;
    Q_FOREACH (const Akonadi::Collection &col, cols) {
        itemStrs.append(EwsClient::folderHash.value(col.remoteId(), ewsHash(col.remoteId())));
    }
    debug.nospace().noquote() << "Akonadi::Collection::List(" << itemStrs.join(QChar::fromLatin1(',')) << ")";
    return debug.maybeSpace();
}

inline QDebug operator<<(QDebug debug, const Akonadi::Collection &col)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "Akonadi::Collection(" << EwsClient::folderHash.value(col.remoteId(), ewsHash(col.remoteId())) << ")";
    return debug.maybeSpace();
}

inline QDebug operator<<(QDebug debug, const QSet<QByteArray> &items)
{
    QDebugStateSaver saver(debug);
    QStringList itemStrs;
    Q_FOREACH (const QByteArray &item, items) {
        itemStrs.append(QString::fromLatin1(item));
    }
    debug.nospace().noquote() << "QSet<QByteArray>(" << itemStrs.join(QChar::fromLatin1(',')) << ")";
    return debug.maybeSpace();
}

#endif
