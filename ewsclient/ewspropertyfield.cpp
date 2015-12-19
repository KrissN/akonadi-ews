/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015 Krzysztof Nowicki <krissn@op.pl>

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

#include <QtCore/QString>

#include "ewspropertyfield.h"
#include "ewsclient_debug.h"

static const QString distinguishedPropSetIdNames[] = {
    QStringLiteral("Meeting"),
    QStringLiteral("Appointment"),
    QStringLiteral("Common"),
    QStringLiteral("PublicStrings"),
    QStringLiteral("Address"),
    QStringLiteral("InternetHeaders"),
    QStringLiteral("CalendarAssistant"),
    QStringLiteral("UnifiedMessaging")
};

static const QString propertyTypeNames[] = {
    QStringLiteral("ApplicationTime"),
    QStringLiteral("ApplicationTimeArray"),
    QStringLiteral("Binary"),
    QStringLiteral("BinaryArray"),
    QStringLiteral("Boolean"),
    QStringLiteral("CLSID"),
    QStringLiteral("CLSIDArray"),
    QStringLiteral("Currency"),
    QStringLiteral("CurrencyArray"),
    QStringLiteral("Double"),
    QStringLiteral("DoubleArray"),
    QStringLiteral("Error"),
    QStringLiteral("Float"),
    QStringLiteral("FloatArray"),
    QStringLiteral("Integer"),
    QStringLiteral("IntegerArray"),
    QStringLiteral("Long"),
    QStringLiteral("LongArray"),
    QStringLiteral("Null"),
    QStringLiteral("Object"),
    QStringLiteral("ObjectArray"),
    QStringLiteral("Short"),
    QStringLiteral("ShortArray"),
    QStringLiteral("SystemTime"),
    QStringLiteral("SystemTimeArray"),
    QStringLiteral("String"),
    QStringLiteral("StringArray")
};

class EwsPropertyFieldPrivate : public QSharedData
{
public:
    EwsPropertyFieldPrivate()
        : mPropType(UnknownField), mIndex(0), mPsIdType(DistinguishedPropSet),
          mPsDid(EwsPropSetMeeting), mIdType(PropName), mId(0), mHasTag(false), mTag(0),
          mType(EwsPropTypeNull), mHash(0)
    {};

    enum Type {
        Field,
        ExtendedField,
        IndexedField,
        UnknownField
    };

    enum PropSetIdType {
        DistinguishedPropSet,
        RealPropSet
    };

    enum PropIdType {
        PropName,
        PropId
    };

    Type mPropType;

    QString mUri;
    unsigned mIndex;

    PropSetIdType mPsIdType;
    EwsDistinguishedPropSetId mPsDid;
    QString mPsId;

    PropIdType mIdType;
    unsigned mId;
    QString mName;

    bool mHasTag;
    unsigned mTag;

    EwsPropertyType mType;

    uint mHash;  // Precalculated hash for the qHash() function.

    void recalcHash();
};

void EwsPropertyFieldPrivate::recalcHash()
{
    mHash = 0;
    switch (mPropType) {
    case Field:
        mHash = 0x00000000 | (qHash(mUri) & 0x3FFFFFFF);
        break;
    case IndexedField:
        mHash = 0x80000000 | ((qHash(mUri) ^ mIndex) & 0x3FFFFFFF);
        break;
    case ExtendedField:
        if (mHasTag) {
            mHash = 0x40000000 | mTag;
        }
        else {
            if (mPsIdType == DistinguishedPropSet) {
                mHash |= mPsDid << 16;
            }
            else {
                mHash |= (qHash(mPsId) & 0x1FFF) << 16;
            }

            if (mIdType == PropId) {
                mHash |= mId & 0xFFFF;
            }
            else {
                mHash |= (qHash(mName) & 0xFFFF);
            }
            mHash |= 0xC0000000;
        }
        break;
    default:
        break;
    }
}

EwsPropertyField::EwsPropertyField()
    : d(new EwsPropertyFieldPrivate())
{
}

EwsPropertyField::EwsPropertyField(QString uri)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::Field;
    d->mUri = uri;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(QString uri, unsigned index)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::IndexedField;
    d->mUri = uri;
    d->mIndex = index;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(EwsDistinguishedPropSetId psid, unsigned id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
    d->mPsDid = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(EwsDistinguishedPropSetId psid, QString name, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
    d->mPsDid = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropName;
    d->mName = name;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(QString psid, unsigned id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::RealPropSet;
    d->mPsId = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(QString psid, QString name, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::RealPropSet;
    d->mPsId = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropName;
    d->mName = name;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(unsigned tag, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mHasTag = true;
    d->mTag = tag;

    d->mType = type;
    d->recalcHash();
}

EwsPropertyField::EwsPropertyField(const EwsPropertyField &other)
    : d(other.d)
{
}

EwsPropertyField::EwsPropertyField(EwsPropertyField &&other)
    : d(other.d)
{
}

EwsPropertyField& EwsPropertyField::operator=(EwsPropertyField &&other)
{
    d = other.d;
    return *this;
}

EwsPropertyField::~EwsPropertyField()
{
}

EwsPropertyField& EwsPropertyField::operator=(const EwsPropertyField &other)
{
    d = other.d;
    return *this;
}

bool EwsPropertyField::operator==(const EwsPropertyField &other) const
{
    if (d == other.d)
        return true;

    const EwsPropertyFieldPrivate *od = other.d;

    if (d->mPropType != od->mPropType)
        return false;

    switch (d->mPropType)
    {
    case EwsPropertyFieldPrivate::UnknownField:
        return true;
    case EwsPropertyFieldPrivate::Field:
        return (d->mUri == od->mUri);
    case EwsPropertyFieldPrivate::IndexedField:
        return (d->mUri == od->mUri) && (d->mIndex == od->mIndex);
    case EwsPropertyFieldPrivate::ExtendedField:
        if (d->mType != od->mType)
            return false;

        if (d->mHasTag != od->mHasTag)
            return false;
        else if (d->mHasTag && d->mTag == od->mTag)
            return true;

        if (d->mPsIdType != od->mPsIdType)
            return false;
        else if ((d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet)
                        && (d->mPsDid != od->mPsDid))
            return false;
        else if ((d->mPsIdType == EwsPropertyFieldPrivate::RealPropSet) && (d->mPsId != od->mPsId))
            return false;

        if (d->mIdType != od->mIdType)
            return false;
        else if ((d->mIdType == EwsPropertyFieldPrivate::PropId) && (d->mId != od->mId))
            return false;
        else if ((d->mIdType == EwsPropertyFieldPrivate::PropName) && (d->mName != od->mName))
            return false;
        return true;
    default:
        return false;
    }

    // Shouldn't get here.
    return false;
}

void EwsPropertyField::write(QXmlStreamWriter &writer) const
{
    switch (d->mPropType)
    {
    case EwsPropertyFieldPrivate::Field:
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("FieldURI"));
        writer.writeAttribute(QStringLiteral("FieldURI"), d->mUri);
        writer.writeEndElement();
        break;
    case EwsPropertyFieldPrivate::IndexedField:
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("IndexedFieldURI"));
        writer.writeAttribute(QStringLiteral("FieldURI"), d->mUri);
        writer.writeAttribute(QStringLiteral("FieldIndex"), QString::number(d->mIndex));
        writer.writeEndElement();
        break;
    case EwsPropertyFieldPrivate::ExtendedField:
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("ExtendedFieldURI"));
        if (d->mHasTag) {
            writer.writeAttribute(QStringLiteral("PropertyTag"),
                                  QStringLiteral("0x") + QString::number(d->mTag, 16));
        }
        else {
            if (d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet) {
                writer.writeAttribute(QStringLiteral("DistinguishedPropertySetId"),
                                      distinguishedPropSetIdNames[d->mPsDid]);
            }
            else {
                writer.writeAttribute(QStringLiteral("PropertySetId"), d->mPsId);
            }

            if (d->mIdType == EwsPropertyFieldPrivate::PropId) {
                writer.writeAttribute(QStringLiteral("PropertyId"),
                                      QStringLiteral("0x") + QString::number(d->mId, 16));
            }
            else {
                writer.writeAttribute(QStringLiteral("PropertyName"), d->mName);
            }
        }
        writer.writeAttribute(QStringLiteral("PropertyType"), propertyTypeNames[d->mType]);
        writer.writeEndElement();
        break;
    case EwsPropertyFieldPrivate::UnknownField:
        break;
    }
}

bool EwsPropertyField::read(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - invalid namespace.");
        return false;
    }

    QXmlStreamAttributes attrs = reader.attributes();
    bool ok;

    // First check the property type
    if (reader.name() == QStringLiteral("FieldURI")) {
        if (!attrs.hasAttribute(QStringLiteral("FieldURI"))) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.")
                            .arg(QStringLiteral("FieldURI"));
            return false;
        }
        d->mPropType = EwsPropertyFieldPrivate::Field;
        d->mUri = attrs.value(QStringLiteral("FieldURI")).toString();
    }
    else if (reader.name() == QStringLiteral("IndexedFieldURI")) {
        if (!attrs.hasAttribute(QStringLiteral("FieldURI"))) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.")
                            .arg(QStringLiteral("FieldURI"));
            return false;
        }
        if (!attrs.hasAttribute(QStringLiteral("FieldIndex"))) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.")
                            .arg(QStringLiteral("FieldIndex"));
            return false;
        }
        unsigned index = attrs.value(QStringLiteral("FieldIndex")).toUInt(&ok, 0);
        if (!ok) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.")
                            .arg(QStringLiteral("FieldIndex"));
            return false;
        }
        d->mPropType = EwsPropertyFieldPrivate::IndexedField;
        d->mUri = attrs.value(QStringLiteral("FieldURI")).toString();
        d->mIndex = index;
    }
    else if (reader.name() == QStringLiteral("ExtendedFieldURI")) {
        if (!attrs.hasAttribute(QStringLiteral("PropertyType"))) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing %1 attribute.")
                            .arg(QStringLiteral("PropertyType"));
            return false;
        }
        QStringRef propTypeText = attrs.value(QStringLiteral("PropertyType"));
        unsigned i;
        EwsPropertyType propType;
        for (i = 0; i < sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0]); i++) {
            if (propTypeText == propertyTypeNames[i]) {
                propType = static_cast<EwsPropertyType>(i);
                break;
            }
        }
        if (i == sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0])) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.")
                            .arg(QStringLiteral("PropertyType"));
            return false;
        }

        if (attrs.hasAttribute(QStringLiteral("PropertyTag"))) {

            unsigned tag = attrs.value(QStringLiteral("PropertyTag")).toUInt(&ok, 0);
            if (!ok) {
                qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.")
                                .arg(QStringLiteral("PropertyTag"));
                return false;
            }
            d->mHasTag = true;
            d->mTag = tag;
        }
        else {
            EwsPropertyFieldPrivate::PropSetIdType psIdType;
            EwsDistinguishedPropSetId psDid;
            QString psId;

            EwsPropertyFieldPrivate::PropIdType idType;
            unsigned id;
            QString name;
            if (attrs.hasAttribute(QStringLiteral("PropertyId"))) {
                id = attrs.value(QStringLiteral("PropertyId")).toUInt(&ok, 0);
                if (!ok) {
                    qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.")
                                    .arg(QStringLiteral("PropertyId"));
                    return false;
                }
                idType = EwsPropertyFieldPrivate::PropId;
            }
            else if (attrs.hasAttribute(QStringLiteral("PropertyName"))) {
                name = attrs.value(QStringLiteral("PropertyName")).toString();
                idType = EwsPropertyFieldPrivate::PropName;
            }
            else {
                qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing one of %1 or %2 attributes.")
                                .arg(QStringLiteral("PropertyId").arg(QStringLiteral("PropertyName")));
                return false;
            }

            if (attrs.hasAttribute(QStringLiteral("DistinguishedPropertySetId"))) {
                QStringRef didText = attrs.value(QStringLiteral("DistinguishedPropertySetId"));
                unsigned i;
                for (i = 0; i < sizeof(distinguishedPropSetIdNames) / sizeof(distinguishedPropSetIdNames[0]); i++) {
                    if (didText == distinguishedPropSetIdNames[i]) {
                        psDid = static_cast<EwsDistinguishedPropSetId>(i);
                        break;
                    }
                }
                if (i == sizeof(distinguishedPropSetIdNames) / sizeof(distinguishedPropSetIdNames[0])) {
                    qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - error reading %1 attribute.")
                                    .arg(QStringLiteral("DistinguishedPropertySetId"));
                    return false;
                }
                psIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
            }
            else if (attrs.hasAttribute(QStringLiteral("PropertySetId"))) {
                psId = attrs.value(QStringLiteral("PropertySetId")).toString();
                psIdType = EwsPropertyFieldPrivate::RealPropSet;
            }
            else {
                qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading property field - missing one of %1 or %2 attributes.")
                                .arg(QStringLiteral("DistinguishedPropertySetId").arg(QStringLiteral("PropertySetId")));
                return false;
            }
            d->mPsIdType = psIdType;
            d->mPsDid = psDid;
            d->mPsId = psId;
            d->mIdType = idType;
            d->mId = id;
            d->mName = name;
        }

        d->mType = propType;
        d->mPropType = EwsPropertyFieldPrivate::ExtendedField;
    }
    d->recalcHash();
    return true;
}

uint qHash(const EwsPropertyField &prop, uint seed)
{
    return prop.d->mHasTag ^ seed;
}

QDebug operator<<(QDebug debug, const EwsPropertyField &prop)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsPropertyField(");

    switch (prop.d->mPropType)
    {
    case EwsPropertyFieldPrivate::Field:
        d << QStringLiteral("FieldUri: ") << prop.d->mUri;
        break;
    case EwsPropertyFieldPrivate::IndexedField:
        d << QStringLiteral("IndexedFieldUri: ") << prop.d->mUri << '@' << prop.d->mIndex;
        break;
    case EwsPropertyFieldPrivate::ExtendedField:
        d << QStringLiteral("ExtendedFieldUri: ");
        if (prop.d->mHasTag) {
            d << QStringLiteral("tag: 0x") << QString::number(prop.d->mTag, 16);
        }
        else {
            if (prop.d->mPsIdType == EwsPropertyFieldPrivate::DistinguishedPropSet) {
                d << QStringLiteral("psdid: ") << distinguishedPropSetIdNames[prop.d->mPsDid];
            }
            else {
                d << QStringLiteral("psid: ") << prop.d->mPsId;
            }
            d << QStringLiteral(", ");

            if (prop.d->mIdType == EwsPropertyFieldPrivate::PropId) {
                d << QStringLiteral("id: 0x") << QString::number(prop.d->mId, 16);
            }
            else {
                d << QStringLiteral("name: ") << prop.d->mName;
            }
        }
        d << QStringLiteral(", type: ") << propertyTypeNames[prop.d->mType];
        break;
    case EwsPropertyFieldPrivate::UnknownField:
        d << QStringLiteral("Unknown");
        break;
    }
    d << ')';
    return debug;
}

