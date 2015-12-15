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
          mType(EwsPropTypeNull)
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
    int mIndex;

    PropSetIdType mPsIdType;
    EwsDistinguishedPropSetId mPsDid;
    QString mPsId;

    PropIdType mIdType;
    int mId;
    QString mName;

    bool mHasTag;
    int mTag;

    EwsPropertyType mType;
};

EwsPropertyField::EwsPropertyField()
    : d(new EwsPropertyFieldPrivate())
{
}

EwsPropertyField::EwsPropertyField(QString uri)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::Field;
    d->mUri = uri;
}

EwsPropertyField::EwsPropertyField(QString uri, int index)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::IndexedField;
    d->mUri = uri;
    d->mIndex = index;
}

EwsPropertyField::EwsPropertyField(EwsDistinguishedPropSetId psid, int id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::DistinguishedPropSet;
    d->mPsDid = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
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
}

EwsPropertyField::EwsPropertyField(QString psid, int id, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mPsIdType = EwsPropertyFieldPrivate::RealPropSet;
    d->mPsId = psid;

    d->mIdType = EwsPropertyFieldPrivate::PropId;
    d->mId = id;

    d->mType = type;
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
}

EwsPropertyField::EwsPropertyField(int tag, EwsPropertyType type)
    : d(new EwsPropertyFieldPrivate())
{
    d->mPropType = EwsPropertyFieldPrivate::ExtendedField;

    d->mHasTag = true;
    d->mTag = tag;

    d->mType = type;
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

bool EwsPropertyField::operator==(const EwsPropertyField &other)
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
        if (d->mPropType != od->mPropType)
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
            writer.writeAttribute(QStringLiteral("PropertyTag"), QString::number(d->mTag));
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
                writer.writeAttribute(QStringLiteral("PropertyId"), QString::number(d->mId));
            }
            else {
                writer.writeAttribute(QStringLiteral("PropertyName"), d->mName);
            }
        }
        writer.writeAttribute(QStringLiteral("PropertyTag"), propertyTypeNames[d->mType]);
        writer.writeEndElement();
        break;
    case EwsPropertyFieldPrivate::UnknownField:
        break;
    }
}