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

#ifndef EWSTYPES_H
#define EWSTYPES_H

#include <QtCore/QString>

extern const QString soapEnvNsUri;
extern const QString ewsMsgNsUri;
extern const QString ewsTypeNsUri;

typedef enum {
    EwsFolderTypeMail,
    EwsFolderTypeCalendar,
    EwsFolderTypeContacts,
    EwsFolderTypeSearch,
    EwsFolderTypeTasks,
    EwsFolderTypeUnknown,
} EwsFolderType;

typedef enum {
    EwsResponseSuccess = 0,
    EwsResponseWarning,
    EwsResponseError,
    EwsResponseParseError,  // Internal - never returned by an Exchange server
    EwsResponseUnknown      // Internal - never returned by an Exchange server
} EwsResponseClass;

typedef enum {
    EwsDIdCalendar = 0,
    EwsDIdContacts,
    EwsDIdDeletedItems,
    EwsDIdDrafts,
    EwsDIdInbox,
    EwsDIdJournal,
    EwsDIdNotes,
    EwsDIdOutbox,
    EwsDIdSentItems,
    EwsDIdTasks,
    EwsDIdMsgFolderRoot,
    EwsDIdRoot,
    EwsDIdJunkEmail,
    EwsDIdSearchFolders,
    EwsDIdVoiceMail,
    EwsDIdRecoverableItemsRoot,
    EwsDIdRecoverableItemsDeletions,
    EwsDIdRecoverableItemsVersions,
    EwsDIdRecoverableItemsPurges,
    EwsDIdArchiveRoot,
    EwsDIdArchiveMsgFolderRoot,
    EwsDIdArchiveDeletedItems,
    EwsDIdArchiveRecoverableItemsRoot,
    EwsDIdArchiveRecoverableItemsDeletions,
    EwsDIdArchiveRecoverableItemsVersions,
    EwsDIdArchiveRecoverableItemsPurges
} EwsDistinguishedId;

typedef enum {
    EwsShapeIdOnly = 0,
    EwsShapeDefault,
    EwsShapeAllProperties
} EwsBaseShape;

typedef enum {
    EwsPropSetMeeting = 0,
    EwsPropSetAppointment,
    EwsPropSetCommon,
    EwsPropSetPublicStrings,
    EwsPropSetAddress,
    EwsPropSetInternetHeaders,
    EwsPropSetCalendarAssistant,
    EwsPropSetUnifiedMessaging
} EwsDistinguishedPropSetId;

typedef enum {
    EwsPropTypeApplicationTime = 0,
    EwsPropTypeApplicationTimeArray,
    EwsPropTypeBinary,
    EwsPropTypeBinaryArray,
    EwsPropTypeBoolean,
    EwsPropTypeCLSID,
    EwsPropTypeCLSIDArray,
    EwsPropTypeCurrency,
    EwsPropTypeCurrencyArray,
    EwsPropTypeDouble,
    EwsPropTypeDoubleArray,
    EwsPropTypeError,
    EwsPropTypeFloat,
    EwsPropTypeFloatArray,
    EwsPropTypeInteger,
    EwsPropTypeTntegerArray,
    EwsPropTypeLong,
    EwsPropTypeLongArray,
    EwsPropTypeNull,
    EwsPropTypeObject,
    EwsPropTypeObjectArray,
    EwsPropTypeShort,
    EwsPropTypeShortArray,
    EwsPropTypeSystemTime,
    EwsPropTypeSystemTimeArray,
    EwsPropTypeString,
    EwsPropTypeStringArray
} EwsPropertyType;

typedef enum {
    EwsTraversalShallow = 0,
    EwsTraversalDeep,
    EwsTraversalSoftDeleted,
    EwsTraversalAssociated
} EwsTraversalType;

typedef enum {
    EwsItemTypeItem = 0,
    EwsItemTypeMessage,
    EwsItemTypeCalendarItem,
    EwsItemTypeContact,
    EwsItemTypeDistributionList,
    EwsItemTypeMeetingMessage,
    EwsItemTypeMeetingRequest,
    EwsItemTypeMeetingResponse,
    EwsItemTypeMeetingCancellation,
    EwsItemTypeTask,
    EwsItemTypeUnknown
} EwsItemType;

typedef enum {
    EwsItemSensitivityNormal,
    EwsItemSensitivityPersonal,
    EwsItemSensitivityPrivate,
    EwsItemSensitivityConfidential
} EwsItemSensitivity;

/**
 *  @brief List of fields in EWS Item and its descendants
 *
 *  The list is based on the XSD schema and contains duplicates, which were commented out.
 */
typedef enum {
    // Folder
    EwsFolderFieldFolderId,
    EwsFolderFieldParentFolderId,
    EwsFolderFieldFolderClass,
    EwsFolderFieldDisplayName,
    EwsFolderFieldTotalCount,
    EwsFolderFieldChildFolderCount,
    EwsFolderFieldManagedFolderInformation,
    EwsFolderFieldEffectiveRights,
    // Calendar folder
    EwsFolderFieldPermissionSet,
    // Contacts folder
    //EwsFolderFieldPermissionSet,          DUPLICATE
    // Mail folder
    EwsFolderFieldUnreadCount,
    //EwsFolderFieldPermissionSet,          DUPLICATE
    // Search folder
    //EwsFolderFieldUnreadCount,            DUPLICATE
    EwsFolderFieldSearchParameters,
    // Tasks folder
    //EwsFolderFieldUnreadCount,            DUPLICATE

    // Item
    EwsItemFieldMimeContent,
    EwsItemFieldItemId,
    EwsItemFieldParentFolderId,
    EwsItemFieldItemClass,
    EwsItemFieldSubject,
    EwsItemFieldSensitivity,
    EwsItemFieldBody,
    EwsItemFieldAttachments,
    EwsItemFieldDateTimeReceived,
    EwsItemFieldSize,
    EwsItemFieldCategories,
    EwsItemFieldImportance,
    EwsItemFieldInReplyTo,
    EwsItemFieldIsSubmitted,
    EwsItemFieldIsDraft,
    EwsItemFieldIsFromMe,
    EwsItemFieldIsResend,
    EwsItemFieldIsUnmodified,
    EwsItemFieldInternetMessageHeaders,
    EwsItemFieldDateTimeSent,
    EwsItemFieldDateTimeCreated,
    EwsItemFieldResponseObjects,
    EwsItemFieldReminderDueBy,
    EwsItemFieldReminderIsSet,
    EwsItemFieldReminderMinutesBeforeStart,
    EwsItemFieldDisplayCc,
    EwsItemFieldDisplayTo,
    EwsItemFieldHasAttachments,
    EwsItemFieldCulture,
    EwsItemFieldEffectiveRights,
    EwsItemFieldLastModifiedName,
    EwsItemFieldLastModifiedTime,
    // Message
    EwsItemFieldSender,
    EwsItemFieldToRecipients,
    EwsItemFieldCcRecipients,
    EwsItemFieldBccRecipients,
    EwsItemFieldIsReadReceiptRequested,
    EwsItemFieldIsDeliveryReceiptRequested,
    EwsItemFieldConversationIndex,
    EwsItemFieldConversationTopic,
    EwsItemFieldFrom,
    EwsItemFieldInternetMessageId,
    EwsItemFieldIsRead,
    EwsItemFieldIsResponseRequested,
    EwsItemFieldReferences,
    EwsItemFieldReplyTo,
    EwsItemFieldReceivedBy,
    EwsItemFieldReceivedRepresenting,
    // Task
    EwsItemFieldActualWork,
    EwsItemFieldAssignedTime,
    EwsItemFieldBillingInformation,
    EwsItemFieldChangeCount,
    EwsItemFieldCompanies,
    EwsItemFieldCompleteDate,
    EwsItemFieldContacts,
    EwsItemFieldDelegationState,
    EwsItemFieldDelegator,
    EwsItemFieldDueDate,
    EwsItemFieldIsAssignmentEditable,
    EwsItemFieldIsComplete,
    EwsItemFieldIsRecurring,
    EwsItemFieldIsTeamTask,
    EwsItemFieldMileage,
    EwsItemFieldOwner,
    EwsItemFieldPercentComplete,
    EwsItemFieldRecurrence,
    EwsItemFieldStartDate,
    EwsItemFieldStatus,
    EwsItemFieldStatusDescription,
    EwsItemFieldTotalWork,
    // Calendar
    EwsItemFieldUID,
    EwsItemFieldRecurrenceId,
    EwsItemFieldDateTimeStamp,
    EwsItemFieldStart,
    EwsItemFieldEnd,
    EwsItemFieldOriginalStart,
    EwsItemFieldIsAllDayEvent,
    EwsItemFieldLegacyFreeBusyStatus,
    EwsItemFieldLocation,
    EwsItemFieldWhen,
    EwsItemFieldIsMeeting,
    EwsItemFieldIsCancelled,
    //EwsItemFieldIsRecurring,              DUPLICATE
    EwsItemFieldMeetingRequestWasSent,
    //EwsItemFieldIsResponseRequested,      DUPLICATE
    EwsItemFieldCalendarItemType,
    EwsItemFieldMyResponseType,
    EwsItemFieldOrganizer,
    EwsItemFieldRequiredAttendees,
    EwsItemFieldOptionalAttendees,
    EwsItemFieldResources,
    EwsItemFieldConflictingMeetingCount,
    EwsItemFieldAdjacentMeetingCount,
    EwsItemFieldConflictingMeetings,
    EwsItemFieldAdjacentMeetings,
    EwsItemFieldDuration,
    EwsItemFieldTimeZone,
    EwsItemFieldStartTimeZone,
    EwsItemFieldEndTimeZone,
    EwsItemFieldAppointmentReplyTime,
    EwsItemFieldAppointmentSequenceNumber,
    EwsItemFieldAppointmentState,
    //EwsItemFieldRecurrence,               DUPLICATE
    EwsItemFieldFirstOccurrence,
    EwsItemFieldLastOccurrence,
    EwsItemFieldModifiedOccurrences,
    EwsItemFieldDeletedOccurrences,
    EwsItemFieldMeetingTimeZone,
    EwsItemFieldConferenceType,
    EwsItemFieldAllowNewTimeProposal,
    EwsItemFieldIsOnlineMeeting,
    EwsItemFieldMeetingWorkspaceUrl,
    EwsItemFieldNetShowUrl,
    // MeetingMessage
    EwsItemFieldAssociatedCalendarItemId,
    EwsItemFieldIsDelegated,
    EwsItemFieldIsOutOfDate,
    EwsItemFieldHasBeenProcessed,
    EwsItemFieldResponseType,
    //EwsItemFieldUID,                      DUPLICATE
    //EwsItemFieldRecurrenceId,             DUPLICATE
    //EwsItemFieldDateTimeStamp,            DUPLICATE
    // MeetingRequestMessage
    EwsItemFieldMeetingRequestType,
    EwsItemFieldIntendedFreeBusyStatus,
    //EwsItemFieldStart,                    DUPLICATE
    //EwsItemFieldEnd,                      DUPLICATE
    //EwsItemFieldOriginalStart,            DUPLICATE
    //EwsItemFieldIsAllDayEvent,            DUPLICATE
    //EwsItemFieldLegacyFreeBusyStatus,     DUPLICATE
    //EwsItemFieldLocation,                 DUPLICATE
    //EwsItemFieldWhen,                     DUPLICATE
    //EwsItemFieldIsMeeting,                DUPLICATE
    //EwsItemFieldIsCancelled,              DUPLICATE
    //EwsItemFieldIsRecurring,              DUPLICATE
    //EwsItemFieldMeetingRequestWasSent,    DUPLICATE
    //EwsItemFieldCalendarItemType,         DUPLICATE
    //EwsItemFieldMyResponseType,           DUPLICATE
    //EwsItemFieldOrganizer,                DUPLICATE
    //EwsItemFieldRequiredAttendees,        DUPLICATE
    //EwsItemFieldOptionalAttendees,        DUPLICATE
    //EwsItemFieldResources,                DUPLICATE
    //EwsItemFieldConflictingMeetingCount,  DUPLICATE
    //EwsItemFieldAdjacentMeetingCount,     DUPLICATE
    //EwsItemFieldConflictingMeetings,      DUPLICATE
    //EwsItemFieldAdjacentMeetings,         DUPLICATE
    //EwsItemFieldDuration,                 DUPLICATE
    //EwsItemFieldTimeZone,                 DUPLICATE
    //EwsItemFieldAppointmentReplyTime,     DUPLICATE
    //EwsItemFieldAppointmentSequenceNumber,DUPLICATE
    //EwsItemFieldAppointmentState,         DUPLICATE
    //EwsItemFieldRecurrence,               DUPLICATE
    //EwsItemFieldFirstOccurrence,          DUPLICATE
    //EwsItemFieldLastOccurrence,           DUPLICATE
    //EwsItemFieldModifiedOccurrences,      DUPLICATE
    //EwsItemFieldDeletedOccurrences,       DUPLICATE
    //EwsItemFieldMeetingTimeZone,          DUPLICATE
    //EwsItemFieldConferenceType,           DUPLICATE
    //EwsItemFieldAllowNewTimeProposal,     DUPLICATE
    //EwsItemFieldIsOnlineMeeting,          DUPLICATE
    //EwsItemFieldMeetingWorkspaceUrl,      DUPLICATE
    //EwsItemFieldNetShowUrl,               DUPLICATE
    // Contact
    EwsItemFieldFileAs,
    EwsItemFieldFileAsMapping,
    EwsItemFieldDisplayName,
    EwsItemFieldGivenName,
    EwsItemFieldInitials,
    EwsItemFieldMiddleName,
    EwsItemFieldNickname,
    EwsItemFieldCompleteName,
    EwsItemFieldCompanyName,
    EwsItemFieldEmailAddresses,
    EwsItemFieldPhysicalAddresses,
    EwsItemFieldPhoneNumbers,
    EwsItemFieldAssistantName,
    EwsItemFieldBirthday,
    EwsItemFieldBusinessHomePage,
    EwsItemFieldChildren,
    //EwsItemFieldCompanies,                DUPLICATE
    EwsItemFieldContactSource,
    EwsItemFieldDepartment,
    EwsItemFieldGeneration,
    EwsItemFieldImAddresses,
    EwsItemFieldJobTitle,
    EwsItemFieldManager,
    //EwsItemFieldMileage,                  DUPLICATE
    EwsItemFieldOfficeLocation,
    EwsItemFieldPostalAddressIndex,
    EwsItemFieldProfession,
    EwsItemFieldSpouseName,
    EwsItemFieldSurname,
    EwsItemFieldWeddingAnniversary,
    // DistributionList
    //EwsItemFieldDisplayName,              DUPLICATE
    //EwsItemFieldFileAs,                   DUPLICATE
    //EwsItemFieldContactSource,            DUPLICATE
    // Additional fields not in EWS specification
    EwsItemFieldBodyIsHtml,
    EwsItemFieldExtendedProperties,
} EwsItemFields;

typedef enum {
    EwsItemImportanceLow,
    EwsItemImportanceNormal,
    EwsItemImportanceHigh
} EwsItemImportance;

typedef enum {
    EwsBasePointBeginning,
    EwsBasePointEnd
} EwsIndexedViewBasePoint;

typedef enum {
    EwsCalendarItemSingle = 0,
    EwsCalendarItemOccurrence,
    EwsCalendarItemException,
    EwsCalendarItemRecurringMaster
} EwsCalendarItemType;

typedef enum {
    EwsEventResponseUnknown = 0,
    EwsEventResponseOrganizer,
    EwsEventResponseTentative,
    EwsEventResponseAccept,
    EwsEventResponseDecline,
    EwsEventResponseNotReceived
} EwsEventResponseType;

typedef enum {
    EwsLfbStatusFree = 0,
    EwsLfbStatusTentative,
    EwsLfbStatusBusy,
    EwsLfbOutOfOffice,
    EwsLfbNoData
} EwsLegacyFreeBusyStatus;

typedef enum {
    EwsDispSaveOnly = 0,
    EwsDispSendOnly,
    EwsDispSendAndSaveCopy,
} EwsMessageDisposition;

typedef enum {
    EwsResolNeverOverwrite = 0,
    EwsResolAutoResolve,
    EwsResolAlwaysOverwrite,
} EwsConflictResolution;

typedef enum {
    EwsMeetingDispSendToNone = 0,
    EwsMeetingDispSendOnlyToAll,
    EwsMeetingDispSendOnlyToChanged,
    EwsMeetingDispSendToAllAndSaveCopy,
    EwsMeetingDispSendToChangedAndSaveCopy,
    EwsMeetingDispUnspecified
} EwsMeetingDisposition;

typedef enum {
    EwsCopiedEvent = 0,
    EwsCreatedEvent,
    EwsDeletedEvent,
    EwsModifiedEvent,
    EwsMovedEvent,
    EwsNewMailEvent,
    EwsFreeBusyChangedEvent
} EwsEventType;

template <typename T> T decodeEnumString(QString str, const QString* table, unsigned count, bool *ok)
{
    unsigned i;
    T enumVal;
    for (i = 0; i < count; i++) {
        if (str == table[i]) {
            enumVal = static_cast<T>(i);
            break;
        }
    }
    *ok = (i < count);
    return enumVal;
}

#endif
