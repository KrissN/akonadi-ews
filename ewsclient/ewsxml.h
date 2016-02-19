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


template <typename T> class EwsXml
{
public:
    typedef std::function<bool(QXmlStreamReader&,QVariant&)> ReadFunction;
    typedef std::function<bool(QXmlStreamWriter&,const QVariant&)> WriteFunction;
    typedef std::function<bool(QXmlStreamReader&,const QString&)> UnknownElementFunction;

    typedef QHash<T, QVariant> ValueHash;

    static Q_CONSTEXPR T Ignore = static_cast<T>(-1);

    struct Item {
        Item() : key(Ignore) {};
        Item(T k, QString n, ReadFunction rfn = ReadFunction(), WriteFunction wfn = WriteFunction())
            : key(k), elmName(n), readFn(rfn), writeFn(wfn) {};
        T key;
        QString elmName;
        ReadFunction readFn;
        WriteFunction writeFn;
    };

    EwsXml() {};
    EwsXml(const QVector<Item> &items) : mItems(items) {
        rebuildItemHash();
    };
    EwsXml(const EwsXml &other)
        : mItems(other.mItems), mValues(other.mValues), mItemHash(other.mItemHash) {};

    void setItems(const QVector<Item> &items) {
        mItems = items;
        rebuildItemHash();
    };

    bool readItem(QXmlStreamReader &reader, QString parentElm, const QString &nsUri,
                  UnknownElementFunction unknownElmFn = &defaultUnknownElmFunction)
    {
        typename QHash<QString, Item>::iterator it = mItemHash.find(reader.name().toString());
        if (it != mItemHash.end() && nsUri == reader.namespaceUri()) {
            if (it->key == Ignore) {
                qCInfoNC(EWSRES_LOG) << QStringLiteral("Unsupported %1 child element %2 - ignoring.")
                                .arg(parentElm).arg(reader.name().toString());
                reader.skipCurrentElement();
                return true;
            }
            else if (!it->readFn) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - no read support for %2 element.")
                                        .arg(parentElm).arg(reader.name().toString());
                return false;
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
        return unknownElmFn(reader, parentElm);
    }

    bool readItems(QXmlStreamReader &reader, const QString &nsUri,
                   UnknownElementFunction unknownElmFn = &defaultUnknownElmFunction)
    {
        QString elmName(reader.name().toString());
        while (reader.readNextStartElement()) {
            if (!readItem(reader, elmName, nsUri, unknownElmFn)) {
                return false;
            }
        }
        return true;
    }

    bool writeItems(QXmlStreamWriter &writer, const QString &parentElm, const QString &nsUri,
                    const ValueHash &values, const QList<T> &keysToWrite = QList<T>()) const
    {
        bool hasKeysToWrite = !keysToWrite.isEmpty();
        Q_FOREACH(const Item& item, mItems) {
            if (!hasKeysToWrite || keysToWrite.contains(item.key)) {
                typename ValueHash::const_iterator it = values.find(item.key);
                if (it != values.end()) {
                    if (!item.writeFn) {
                        qCWarning(EWSRES_LOG) << QStringLiteral("Failed to write %1 element - no write support for %2 element.")
                                                .arg(parentElm).arg(item.elmName);
                        return false;
                    }
                    writer.writeStartElement(nsUri, item.elmName);
                    bool status = item.writeFn(writer, *it);
                    writer.writeEndElement();
                    if (!status) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    ValueHash values() const {
        return mValues;
    }

private:
    static bool defaultUnknownElmFunction(QXmlStreamReader &reader, const QString &parentElm)
    {
        qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid %2 element.")
                                .arg(parentElm).arg(reader.name().toString());
        return false;
    }

    const QVector<Item> mItems;
    ValueHash mValues;
    QHash<QString, Item> mItemHash;

    void rebuildItemHash() {
        Q_FOREACH(const Item &item, mItems) {
            mItemHash.insert(item.elmName, item);
        }
    }
};

extern bool ewsXmlBoolReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlBoolWriter(QXmlStreamWriter &writer, const QVariant &val);
extern bool ewsXmlBase64Reader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlBase64Writer(QXmlStreamWriter &writer, const QVariant &val);
extern bool ewsXmlIdReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlIdWriter(QXmlStreamWriter &writer, const QVariant &val);
extern bool ewsXmlTextReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlTextWriter(QXmlStreamWriter &writer, const QVariant &val);
extern bool ewsXmlUIntReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlUIntWriter(QXmlStreamWriter &writer, const QVariant &val);
extern bool ewsXmlDateTimeReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlItemReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlFolderReader(QXmlStreamReader &reader, QVariant &val);

extern bool ewsXmlEnumReader(QXmlStreamReader &reader, QVariant &val, QVector<QString> items);
extern bool ewsXmlSensitivityReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlImportanceReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlCalendarItemTypeReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlLegacyFreeBusyStatusReader(QXmlStreamReader &reader, QVariant &val);
extern bool ewsXmlResponseTypeReader(QXmlStreamReader &reader, QVariant &val);


#endif
