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

#ifndef EWSXMLREADER_H
#define EWSXMLREADER_H

#include <functional>
#include <QtCore/QVector>
#include <QtCore/QXmlStreamReader>

#include "ewsclient_debug.h"


template <typename T> class EwsXmlReader
{
public:
    typedef std::function<bool(QXmlStreamReader&,QVariant&)> ReadFunction;

    static Q_CONSTEXPR T Ignore = static_cast<T>(-1);

    struct Item {
        Item() : key(Ignore) {};
        Item(T k, QString n, QString ns, ReadFunction fn = ReadFunction())
            : key(k), elmName(n), ns(ns), readFn(fn) {};
        T key;
        QString elmName;
        QString ns;
        ReadFunction readFn;
    };

    EwsXmlReader() {};
    EwsXmlReader(const QVector<Item> &items) : mItems(items) {
        rebuildItemHash();
    };
    EwsXmlReader(const EwsXmlReader &other)
        : mItems(other.mItems), mValues(other.mValues), mItemHash(other.mItemHash) {};

    void setItems(const QVector<Item> &items) {
        mItems = items;
        rebuildItemHash();
    };

    bool readItem(QXmlStreamReader &reader, QString parentElm)
    {
        typename QHash<QString, Item>::iterator it = mItemHash.find(reader.name().toString());
        if (it != mItemHash.end() && it->ns == reader.namespaceUri()) {
            if (it->key == Ignore) {
                qCInfoNC(EWSRES_LOG) << QStringLiteral("Unsupported %1 child element %2 - ignoring.")
                                .arg(parentElm).arg(reader.name().toString());
                reader.skipCurrentElement();
                return true;
            }
            else {
                QVariant val = mValues[it->key];
                if (it->readFn(reader, val)) {
                    mValues[it->key] = val;
                    return true;
                }
                return false;
            }
        }
        qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                        .arg(parentElm).arg(reader.name().toString());

        return false;
    }

    bool readItems(QXmlStreamReader &reader) {

        return true;
    }

    QHash<T, QVariant> values() const {
        return mValues;
    }

private:
    const QVector<Item> mItems;
    QHash<T, QVariant> mValues;
    QHash<QString, Item> mItemHash;

    void rebuildItemHash() {
        Q_FOREACH(const Item &item, mItems) {
            mItemHash.insert(item.elmName, item);
        }
        qDebug() << mItemHash.size();
    }
};

extern bool ewsXmlBoolReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlBase64Reader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlIdReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlTextReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlUIntReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlDateTimeReader(QXmlStreamReader &reader, QVariant &val);

extern bool ewsXmlEnumReader(QXmlStreamReader &reader, QVariant &val, QVector<QString> items);
extern bool ewsXmlSensitivityReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlImportanceReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlCalendarItemTypeReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlLegacyFreeBusyStatusReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlResponseTypeReader(QXmlStreamReader &reader, QVariant &val);

#endif
