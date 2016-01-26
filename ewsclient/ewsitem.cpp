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

#include <ewsxmlreader.h>
#include "ewsitem.h"

#include <KCodecs/KCodecs>

#include "ewsitembase_p.h"
#include "ewsmailbox.h"
#include "ewsattendee.h"
#include "ewsrecurrence.h"
#include "ewsoccurrence.h"
#include "ewsxmlreader.h"
#include "ewsclient_debug.h"

class EwsItemPrivate : public EwsItemBasePrivate
{
public:
    typedef EwsXmlReader<EwsItemFields> Reader;

    EwsItemPrivate();
    EwsItemPrivate(const EwsItemBasePrivate &other);
    virtual EwsItemBasePrivate *clone() const Q_DECL_OVERRIDE
    {
        return new EwsItemPrivate(*this);
    }

    static bool bodyReader(QXmlStreamReader &reader, QVariant &val);
    static bool messageHeadersReader(QXmlStreamReader &reader, QVariant &val);
    static bool mailboxReader(QXmlStreamReader &reader, QVariant &val);
    static bool recipientsReader(QXmlStreamReader &reader, QVariant &val);
    static bool timezoneReader(QXmlStreamReader &reader, QVariant &val);
    static bool attendeesReader(QXmlStreamReader &reader, QVariant &val);
    static bool occurrenceReader(QXmlStreamReader &reader, QVariant &val);
    static bool occurrencesReader(QXmlStreamReader &reader, QVariant &val);
    static bool recurrenceReader(QXmlStreamReader &reader, QVariant &val);
    static bool categoriesReader(QXmlStreamReader &reader, QVariant &val);

    EwsItemType mType;
    static const Reader mStaticEwsReader;
    Reader mEwsReader;
};

static const QVector<EwsItemPrivate::Reader::Item> ewsItemItems = {
    {EwsItemFieldMimeContent, QStringLiteral("MimeContent"), &ewsXmlBase64Reader},
    {EwsItemFieldItemId, QStringLiteral("ItemId"), &ewsXmlIdReader},
    {EwsItemFieldParentFolderId, QStringLiteral("ParentFolderId"), &ewsXmlIdReader},
    {EwsItemFieldItemClass, QStringLiteral("ItemClass"), &ewsXmlTextReader},
    {EwsItemFieldSubject, QStringLiteral("Subject"), &ewsXmlTextReader},
    {EwsItemFieldSensitivity, QStringLiteral("Sensitivity"), &ewsXmlSensitivityReader},
    {EwsItemFieldBody, QStringLiteral("Body"), &EwsItemPrivate::bodyReader},
    {EwsItemFieldSize, QStringLiteral("Size"), &ewsXmlUIntReader},
    {EwsItemFieldInternetMessageHeaders, QStringLiteral("InternetMessageHeaders"),
        &EwsItemPrivate::messageHeadersReader},
    {EwsItemFieldExtendedProperties, QStringLiteral("ExtendedProperty"),
        &EwsItemBasePrivate::extendedPropertyReader},
    {EwsItemFieldImportance, QStringLiteral("Importance"), &ewsXmlImportanceReader},
    {EwsItemFieldFrom, QStringLiteral("From"), &EwsItemPrivate::mailboxReader},
    {EwsItemFieldToRecipients, QStringLiteral("ToRecipients"), &EwsItemPrivate::recipientsReader},
    {EwsItemFieldCcRecipients, QStringLiteral("CcRecipients"), &EwsItemPrivate::recipientsReader},
    {EwsItemFieldBccRecipients, QStringLiteral("BccRecipients"), &EwsItemPrivate::recipientsReader},
    {EwsItemFieldIsRead, QStringLiteral("IsRead"), &ewsXmlBoolReader},
    {EwsItemFieldHasAttachments, QStringLiteral("HasAttachments"), &ewsXmlBoolReader},
    {EwsItemFieldIsFromMe, QStringLiteral("IsFromMe"), &ewsXmlBoolReader},
    {EwsItemFieldCalendarItemType, QStringLiteral("CalendarItemType"),
        &ewsXmlCalendarItemTypeReader},
    {EwsItemFieldUID, QStringLiteral("UID"), &ewsXmlTextReader},
    {EwsItemFieldCulture, QStringLiteral("Culture"), &ewsXmlTextReader},
    {EwsItemFieldStartTimeZone, QStringLiteral("StartTimeZone"), &EwsItemPrivate::timezoneReader},
    {EwsItemFieldOrganizer, QStringLiteral("Organizer"), &EwsItemPrivate::mailboxReader},
    {EwsItemFieldRequiredAttendees, QStringLiteral("RequiredAttendees"),
        &EwsItemPrivate::attendeesReader},
    {EwsItemFieldOptionalAttendees, QStringLiteral("OptionalAttendees"),
        &EwsItemPrivate::attendeesReader},
    {EwsItemFieldResources, QStringLiteral("Resources"), &EwsItemPrivate::attendeesReader},
    {EwsItemFieldStart, QStringLiteral("Start"), &ewsXmlDateTimeReader},
    {EwsItemFieldEnd, QStringLiteral("End"), &ewsXmlDateTimeReader},
    {EwsItemFieldRecurrenceId, QStringLiteral("RecurrenceId"), &ewsXmlDateTimeReader},
    {EwsItemFieldIsAllDayEvent, QStringLiteral("IsAllDayEvent"), &ewsXmlBoolReader},
    {EwsItemFieldLegacyFreeBusyStatus, QStringLiteral("LegacyFreeBusyStatus"),
        &ewsXmlLegacyFreeBusyStatusReader},
    {EwsItemFieldMyResponseType, QStringLiteral("MyResponseType"), &ewsXmlResponseTypeReader},
    {EwsItemFieldAppointmentSequenceNumber, QStringLiteral("AppointmentSequenceNumber"),
        &ewsXmlUIntReader},
    {EwsItemFieldRecurrence, QStringLiteral("Recurrence"), &EwsItemPrivate::recurrenceReader},
    {EwsItemFieldFirstOccurrence, QStringLiteral("FirstOccurrence"),
        &EwsItemPrivate::occurrenceReader},
    {EwsItemFieldLastOccurrence, QStringLiteral("LastOccurrence"),
        &EwsItemPrivate::occurrenceReader},
    {EwsItemFieldModifiedOccurrences, QStringLiteral("ModifiedOccurrences"),
        &EwsItemPrivate::occurrencesReader},
    {EwsItemFieldDeletedOccurrences, QStringLiteral("DeletedOccurrences"),
        &EwsItemPrivate::occurrencesReader},
    {EwsItemFieldCategories, QStringLiteral("Categories"), &EwsItemPrivate::categoriesReader},
};

const EwsItemPrivate::Reader EwsItemPrivate::mStaticEwsReader(ewsItemItems);

EwsItemPrivate::EwsItemPrivate()
    : EwsItemBasePrivate(), mType(EwsItemTypeUnknown), mEwsReader(mStaticEwsReader)
{
}

bool EwsItemPrivate::bodyReader(QXmlStreamReader &reader, QVariant &val)
{
    QVariantList vl;
    QStringRef bodyType = reader.attributes().value(QStringLiteral("BodyType"));
    if (bodyType.isNull()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - missing %2 attribute")
                        .arg(QStringLiteral("Body")).arg(QStringLiteral("BodyType"));
        return false;
    }
    bool isHtml;
    if (bodyType == QStringLiteral("HTML")) {
        isHtml = true;
    }
    else if (bodyType == QStringLiteral("Text")) {
        isHtml = false;
    }
    else {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read Body element- unknown body type");
        return false;
    }
    vl.append(reader.readElementText());
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to read %1 element - invalid content.")
                        .arg(QStringLiteral("Body"));
        return false;
    }
    vl.append(isHtml);
    val = vl;
    return true;
}

bool EwsItemPrivate::messageHeadersReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsItem::HeaderMap map = val.value<EwsItem::HeaderMap>();

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in InternetMessageHeaders element:")
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("InternetMessageHeader")) {
            QStringRef nameRef = reader.attributes().value(QStringLiteral("HeaderName"));
            if (nameRef.isNull()) {
                qCWarningNC(EWSRES_LOG) << QStringLiteral("Missing HeaderName attribute in InternetMessageHeader element.");
                return false;
            }
            QString name = nameRef.toString();
            QString value = reader.readElementText();
            map.insert(name, value);
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                                                        .arg(QStringLiteral("InternetMessageHeader"));
                return false;
            }
            if (name.compare("Message-ID", Qt::CaseInsensitive) == 0) {
                qCDebugNC(EWSRES_LOG) << QStringLiteral("MessageID: %1").arg(value);
            }
        }
    }

    val = QVariant::fromValue<EwsItem::HeaderMap>(map);

    return true;
}

bool EwsItemPrivate::recipientsReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsMailbox::List mboxList;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(reader.name().toString())
                            << reader.namespaceUri();
            return false;
        }
        EwsMailbox mbox(reader);
        if (!mbox.isValid()) {
            return false;
        }
        mboxList.append(mbox);
    }

    val = QVariant::fromValue<EwsMailbox::List>(mboxList);

    return true;
}

bool EwsItemPrivate::mailboxReader(QXmlStreamReader &reader, QVariant &val)
{
    if (!reader.readNextStartElement()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Expected mailbox in %1 element:").arg(reader.name().toString());
        return false;
    }

    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(reader.name().toString())
                            << reader.namespaceUri();
        reader.skipCurrentElement();
        return false;
    }

    EwsMailbox mbox(reader);
    if (!mbox.isValid()) {
        reader.skipCurrentElement();
        return false;
    }

    val = QVariant::fromValue<EwsMailbox>(mbox);

    // Leave the element
    reader.skipCurrentElement();

    return true;
}

bool EwsItemPrivate::timezoneReader(QXmlStreamReader &reader, QVariant &val)
{
    // TODO: This only reads the timezone identifier.
    QStringRef idRef = reader.attributes().value(QStringLiteral("Id"));
    if (idRef.isNull()) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Error reading %1 element - missing %2 attribute")
                        .arg(reader.name().toString()).arg(QStringLiteral("Id"));
        reader.skipCurrentElement();
        return false;
    }
    else {
        reader.skipCurrentElement();
        val = idRef.toString();
        return true;
    }
}

bool EwsItemPrivate::attendeesReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsAttendee::List attList;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(reader.name().toString())
                            << reader.namespaceUri();
            return false;
        }
        EwsAttendee att(reader);
        if (!att.isValid()) {
            return false;
        }
        attList.append(att);
    }

    val = QVariant::fromValue<EwsAttendee::List>(attList);

    return true;
}

bool EwsItemPrivate::occurrenceReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsOccurrence occ(reader);
    if (!occ.isValid()) {
        return false;
    }
    val = QVariant::fromValue<EwsOccurrence>(occ);
    return true;
}

bool EwsItemPrivate::occurrencesReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsOccurrence::List occList;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in %1 element:")
                            .arg(reader.name().toString())
                            << reader.namespaceUri();
            return false;
        }
        EwsOccurrence occ(reader);
        if (!occ.isValid()) {
            return false;
        }
        occList.append(occ);
    }

    val = QVariant::fromValue<EwsOccurrence::List>(occList);

    return true;
}

bool EwsItemPrivate::categoriesReader(QXmlStreamReader &reader, QVariant &val)
{
    QStringList categories;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Unexpected namespace in %1 element:")
                            .arg(reader.name().toString())
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("String")) {
            categories.append(reader.readElementText());
            if (reader.error() !=  QXmlStreamReader::NoError) {
                qCWarning(EWSRES_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                                                        .arg(QStringLiteral("Categories/Value"));
                return false;
            }
        }
    }

    val = categories;

    return true;
}

bool EwsItemPrivate::recurrenceReader(QXmlStreamReader &reader, QVariant &val)
{
    EwsRecurrence recurrence(reader);
    val = QVariant::fromValue<EwsRecurrence>(recurrence);
    return true;
}

EwsItem::EwsItem()
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsItemPrivate()))
{
}

EwsItem::EwsItem(QXmlStreamReader &reader)
    : EwsItemBase(QSharedDataPointer<EwsItemBasePrivate>(new EwsItemPrivate()))
{
    qRegisterMetaType<EwsItem::HeaderMap>();
    qRegisterMetaType<EwsMailbox>();
    qRegisterMetaType<EwsMailbox::List>();
    qRegisterMetaType<EwsAttendee>();
    qRegisterMetaType<EwsAttendee::List>();
    qRegisterMetaType<EwsRecurrence>();
    qRegisterMetaType<EwsOccurrence>();
    qRegisterMetaType<EwsOccurrence::List>();

    EwsItemPrivate *d = reinterpret_cast<EwsItemPrivate*>(this->d.data());

    // Check what item type are we
    if (reader.name() == QStringLiteral("Item")) {
        d->mType = EwsItemTypeItem;
    }
    else if (reader.name() == QStringLiteral("Message")) {
        d->mType = EwsItemTypeMessage;
    }
    else if (reader.name() == QStringLiteral("CalendarItem")) {
        d->mType = EwsItemTypeCalendarItem;
    }
    else if (reader.name() == QStringLiteral("Contact")) {
        d->mType = EwsItemTypeContact;
    }
    else if (reader.name() == QStringLiteral("DistributionList")) {
        d->mType = EwsItemTypeDistributionList;
    }
    else if (reader.name() == QStringLiteral("MeetingMessage")) {
        d->mType = EwsItemTypeMeetingMessage;
    }
    else if (reader.name() == QStringLiteral("MeetingRequest")) {
        d->mType = EwsItemTypeMeetingRequest;
    }
    else if (reader.name() == QStringLiteral("MeetingResponse")) {
        d->mType = EwsItemTypeMeetingResponse;
    }
    else if (reader.name() == QStringLiteral("MeetingCancellation")) {
        d->mType = EwsItemTypeMeetingCancellation;
    }
    else if (reader.name() == QStringLiteral("Task")) {
        d->mType = EwsItemTypeTask;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSRES_LOG) << "Unexpected namespace in Item element:"
                            << reader.namespaceUri();
            return;
        }

        if (!readBaseItemElement(reader)) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid Item child: %1").arg(reader.qualifiedName().toString());
            return;
        }
    }
    d->mValid = true;

}

EwsItem::EwsItem(const EwsItem &other)
    : EwsItemBase(other.d)
{
}

EwsItem::EwsItem(EwsItem &&other)
    : EwsItemBase(other.d)
{
}

EwsItem::~EwsItem()
{
}

EwsItem& EwsItem::operator=(const EwsItem &other)
{
    d = other.d;
    return *this;
}

EwsItem& EwsItem::operator=(EwsItem &&other)
{
    d = std::move(other.d);
    return *this;
}

EwsItemType EwsItem::type() const
{
    const EwsItemPrivate *d = reinterpret_cast<const EwsItemPrivate*>(this->d.data());
    return d->mType;
}

bool EwsItem::readBaseItemElement(QXmlStreamReader &reader)
{
    EwsItemPrivate *d = reinterpret_cast<EwsItemPrivate*>(this->d.data());

    if (!d->mEwsReader.readItem(reader, "Item", ewsTypeNsUri)) {
        return false;
    }

    d->mFields = d->mEwsReader.values();
    d->mProperties = d->mFields[EwsItemFieldExtendedProperties].value<EwsItemBasePrivate::PropertyHash>();

    // The body item is special as it hold two values in one. Need to separate them into their
    // proper places.
    if (d->mFields.contains(EwsItemFieldBody)) {
        QVariantList vl = d->mFields[EwsItemFieldBody].value<QVariantList>();
        QVariantList::const_iterator it = vl.cbegin();
        d->mFields[EwsItemFieldBody] = *it++;
        d->mFields[EwsItemFieldBodyIsHtml] = *it;
    }
    return true;
}

