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

#include "ewsitem.h"

#include <KCodecs/KCodecs>

#include "ewsitembase_p.h"
#include "ewsmailbox.h"
#include "ewsclient_debug.h"

static const QString messageSensitivityNames[] = {
    QStringLiteral("Normal"),
    QStringLiteral("Personal"),
    QStringLiteral("Private"),
    QStringLiteral("Confidential")
};
Q_CONSTEXPR unsigned messageSensitivityNameCount = sizeof(messageSensitivityNames) / sizeof(messageSensitivityNames[0]);

static const QString messageImportanceNames[] = {
    QStringLiteral("Low"),
    QStringLiteral("Normal"),
    QStringLiteral("High")
};
Q_CONSTEXPR unsigned messageImportanceNameCount = sizeof(messageImportanceNames) / sizeof(messageImportanceNames[0]);

static const QString calendarItemTypeNames[] = {
    QStringLiteral("Single"),
    QStringLiteral("Occurrence"),
    QStringLiteral("Exception"),
    QStringLiteral("RecurringMaster")
};
Q_CONSTEXPR unsigned calendarItemTypeNameCount = sizeof(calendarItemTypeNames) / sizeof(calendarItemTypeNames[0]);

class EwsItemPrivate : public EwsItemBasePrivate
{
public:
    EwsItemPrivate();
    EwsItemPrivate(const EwsItemBasePrivate &other);
    virtual EwsItemBasePrivate *clone() const Q_DECL_OVERRIDE
    {
        return new EwsItemPrivate(*this);
    }

    bool readMessageHeaders(QXmlStreamReader &reader);
    bool readMailbox(QXmlStreamReader &reader, EwsItemFields field);
    bool readRecipients(QXmlStreamReader &reader, EwsItemFields field);
    bool readBoolean(QXmlStreamReader &reader, EwsItemFields field);

    EwsItemType mType;
};

EwsItemPrivate::EwsItemPrivate()
    : EwsItemBasePrivate(), mType(EwsItemTypeUnknown)
{
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
            qCWarningNC(EWSCLIENT_LOG) << "Unexpected namespace in Item element:"
                            << reader.namespaceUri();
            return;
        }

        if (!readBaseItemElement(reader)) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Invalid Item child: %1").arg(reader.qualifiedName().toString());
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

    if (reader.name() == QStringLiteral("MimeContent")) {
        d->mFields[EwsItemFieldMimeContent] = KCodecs::base64Decode(reader.readElementText().toLatin1());
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("MimeContent"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ItemId")) {
        EwsId id = EwsId(reader);
        if (id.type() == EwsId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ItemId"));
            return false;
        }
        d->mFields[EwsItemFieldItemId] = QVariant::fromValue(id);
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ParentFolderId")) {
        EwsId id = EwsId(reader);
        if (id.type() == EwsId::Unspecified) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ParentFolderId"));
            return false;
        }
        d->mFields[EwsItemFieldParentFolderId] = QVariant::fromValue(id);
        reader.skipCurrentElement();
    }
    else if (reader.name() == QStringLiteral("ItemClass")) {
        d->mFields[EwsItemFieldItemClass]= reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("ItemClass"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("Subject")) {
        d->mFields[EwsItemFieldSubject] = reader.readElementText();
        qCDebug(EWSCLIENT_LOG) << QStringLiteral("Subject: %1").arg(d->mFields[EwsItemFieldSubject].toString());
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("Subject"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("Sensitivity")) {
        bool ok;
        d->mFields[EwsItemFieldSensitivity] =
                        decodeEnumString<EwsItemSensitivity>(reader.readElementText(),
                            messageSensitivityNames, messageSensitivityNameCount, &ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("Sensitivity"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("Body")) {
        QStringRef bodyType = reader.attributes().value(QStringLiteral("BodyType"));
        if (bodyType.isNull()) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - missing %1 attribute")
                            .arg(QStringLiteral("BodyType"));
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
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - unknown body type");
            return false;
        }
        d->mFields[EwsItemFieldBody] = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("Body"));
            return false;
        }
        d->mFields[EwsItemFieldBodyIsHtml] = isHtml;
    }
    else if (reader.name() == QStringLiteral("Size")) {
        bool ok;
        d->mFields[EwsItemFieldSubject] = reader.readElementText().toUInt(&ok, -1);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("Size"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("InternetMessageHeaders")) {
        if (!d->readMessageHeaders(reader)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ExtendedProperty")) {
        if (!readExtendedProperty(reader)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("Importance")) {
        bool ok;
        d->mFields[EwsItemFieldImportance] =
                        decodeEnumString<EwsItemImportance>(reader.readElementText(),
                            messageImportanceNames, messageImportanceNameCount, &ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("Importance"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("From")) {
        if (!d->readMailbox(reader, EwsItemFieldFrom)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("ToRecipients")) {
        if (!d->readRecipients(reader, EwsItemFieldToRecipients)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("CcRecipients")) {
        if (!d->readRecipients(reader, EwsItemFieldCcRecipients)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("BccRecipients")) {
        if (!d->readRecipients(reader, EwsItemFieldBccRecipients)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("IsRead")) {
        if (!d->readBoolean(reader, EwsItemFieldIsRead)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("HasAttachments")) {
        if (!d->readBoolean(reader, EwsItemFieldHasAttachments)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("IsFromMe")) {
        if (!d->readBoolean(reader, EwsItemFieldIsFromMe)) {
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("CalendarItemType")) {
        bool ok;
        d->mFields[EwsItemFieldCalendarItemType] =
                        decodeEnumString<EwsCalendarItemType>(reader.readElementText(),
                            calendarItemTypeNames, calendarItemTypeNameCount, &ok);
        if (reader.error() != QXmlStreamReader::NoError || !ok) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("CalendarItemType"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("UID")) {
        d->mFields[EwsItemFieldUID] = reader.readElementText();
        if (reader.error() != QXmlStreamReader::NoError) {
            qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                            .arg(QStringLiteral("UID"));
            return false;
        }
    }
    else if (reader.name() == QStringLiteral("Attachments") ||
             reader.name() == QStringLiteral("DateTimeReceived") ||
             reader.name() == QStringLiteral("Categories") ||
             reader.name() == QStringLiteral("InReplyTo") ||
             reader.name() == QStringLiteral("IsSubmitted") ||
             reader.name() == QStringLiteral("IsDraft") ||
             reader.name() == QStringLiteral("IsFromMe") ||
             reader.name() == QStringLiteral("IsResend") ||
             reader.name() == QStringLiteral("IsUnmodified") ||
             reader.name() == QStringLiteral("DateTimeSent") ||
             reader.name() == QStringLiteral("DateTimeCreated") ||
             reader.name() == QStringLiteral("ResponseObjects") ||
             reader.name() == QStringLiteral("ReminderDueBy") ||
             reader.name() == QStringLiteral("ReminderIsSet") ||
             reader.name() == QStringLiteral("ReminderMinutesBeforeStart") ||
             reader.name() == QStringLiteral("DisplayCc") ||
             reader.name() == QStringLiteral("DisplayTo") ||
             reader.name() == QStringLiteral("Culture") ||
             reader.name() == QStringLiteral("EffectiveRights") ||
             reader.name() == QStringLiteral("LastModifiedName") ||
             reader.name() == QStringLiteral("LastModifiedTime")) {
        // Unsupported - ignore
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unsupported Item property %1").arg(reader.name().toString());
        reader.skipCurrentElement();
    }
    else {
        qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - unknown element: %1.")
                        .arg(reader.name().toString());
        return false;
    }
    return true;
}

bool EwsItemPrivate::readMessageHeaders(QXmlStreamReader &reader)
{
    EwsItem::HeaderMap map;

    if (mFields.contains(EwsItemFieldInternetMessageHeaders)) {
        map = mFields[EwsItemFieldInternetMessageHeaders].value<EwsItem::HeaderMap>();
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unexpected namespace in InternetMessageHeaders element:")
                            << reader.namespaceUri();
            return false;
        }

        if (reader.name() == QStringLiteral("InternetMessageHeader")) {
            QStringRef nameRef = reader.attributes().value(QStringLiteral("HeaderName"));
            if (nameRef.isNull()) {
                qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Missing HeaderName attribute in InternetMessageHeader element.");
                return false;
            }
            QString name = nameRef.toString();
            QString value = reader.readElementText();
            map.insert(name, value);
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSCLIENT_LOG) << QStringLiteral("Failed to read EWS request - invalid %1 element.")
                                                        .arg(QStringLiteral("InternetMessageHeader"));
                return false;
            }
            if (name.compare("Message-ID", Qt::CaseInsensitive) == 0) {
                qCDebugNC(EWSCLIENT_LOG) << QStringLiteral("MessageID: %1").arg(value);
            }
        }
    }

    mFields[EwsItemFieldInternetMessageHeaders] = QVariant::fromValue<EwsItem::HeaderMap>(map);

    return true;
}

bool EwsItemPrivate::readRecipients(QXmlStreamReader &reader, EwsItemFields field)
{
    EwsMailbox::List mboxList;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(reader.name().toString())
                            << reader.namespaceUri();
            return false;
        }
        EwsMailbox mbox(reader);
        if (!mbox.isValid()) {
            return false;
        }
        mboxList.append(mbox);
    }

    mFields[field] = QVariant::fromValue<EwsMailbox::List>(mboxList);

    return true;
}

bool EwsItemPrivate::readMailbox(QXmlStreamReader &reader, EwsItemFields field)
{
    if (!reader.readNextStartElement()) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Expected mailbox in %1 element:").arg(reader.name().toString());
        return false;
    }

    if (reader.namespaceUri() != ewsTypeNsUri) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unexpected namespace in %1 element:").arg(reader.name().toString())
                            << reader.namespaceUri();
        return false;
    }

    EwsMailbox mbox(reader);
    if (!mbox.isValid()) {
        return false;
    }

    mFields[field] = QVariant::fromValue<EwsMailbox>(mbox);

    // Leave the element
    reader.skipCurrentElement();

    return true;
}

bool EwsItemPrivate::readBoolean(QXmlStreamReader &reader, EwsItemFields field)
{
    QString elmText = reader.readElementText();
    if (reader.error() != QXmlStreamReader::NoError) {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Error reading %1 element")
                        .arg(reader.name().toString());
        return false;
    }
    bool val = false;
    if (elmText == QStringLiteral("true")) {
        val = true;
    }
    else if (elmText == QStringLiteral("false")) {
        val = false;
    }
    else {
        qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unexpected invalid boolean value in %1 element:")
                        .arg(reader.name().toString())
                        << elmText;
        return false;
    }

    mFields[field] = val;

    return true;
}

