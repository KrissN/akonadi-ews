/*
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

#ifndef EWSTAGSTORE_H
#define EWSTAGSTORE_H

#include <AkonadiCore/Tag>
#include <AkonadiCore/Item>

#include "ewspropertyfield.h"

class EwsItem;

/**
 *  @brief  EWS Tag storage helper class
 *
 *  The role of this class is to maintain a list of tags within an EWS resource and synchronize
 *  the tags between Akonadi items and the Exchange server.
 *
 *  Exchange tag support is rather basic. Tags are not first-class objects like folders or items,
 *  but exist as 'categories' assigned to items. Categories are string labels attached to an item.
 *  Each item can have any number of categories.
 *
 *  Outlook (and OWA) are able to assign colors to tags. This is done outside of Exchange in form
 *  of extra metadata stored in a hidden item in the default Calendar folder. This metadata is
 *  binary and private to Outlook (although some documentation exists about it).
 *
 *  On the Akonadi side tags are first-class citizens and can carry much more information than
 *  Exchange or Outlook private metadata is able to store. In order to store Akonadi tags with all
 *  their content the EWS resource makes use of private properties.
 *
 *  @section tags_global_list Global tag list
 *
 *  Similar to Outlook the Akonadi EWS resource keeps a master list of tags. The list is stored
 *  in a private property in the root mailbox folder. This list contains a list of tags serialized
 *  into Base64 strings.
 *
 *  This list is updated each time tags are added, changed or deleted. It is also retrieved during
 *  startup in order to populate the Akonadi database with tags potentially stored by another
 *  instance of the resource on another machine.
 *
 *  Along with the tag list a version field is stored in form of an integer. Each time the tag list
 *  is updated the version is incremented by one. This field allows another resource instance running
 *  in parallel to detect that the global tag list has changed and synchronize it with Akonadi.
 *
 *  There are no attempts made to synchronize Akonadi and Outlook tag lists.
 *
 *  @section tags_item_list Item tag list
 *
 *  Tags for items are also stored in a private property. The list contains an array of tag unique
 *  identifiers. In parallel to this any update of item tag also populates the category list with
 *  the tag names. This provides a one-way synchronization of tags from Akonadi to Outlook.
 *
 *  @section tags_akonadi_sync Synchronization with Akonadi tags
 *
 *  In the current version of Akonadi tag support is partially broken. One of the problematic issues
 *  is that Akonadi doesn't always tell the resource about the tags it has. This means that the list
 *  of tags can be out of sync with the list on Akonadi side. This causes two problems. One is that
 *  items from Akonadi can have tags that the resource has never seen before. Such tags will need
 *  to be fetched with their full content and written to the server before they can be attached to
 *  the item. Another problem is in the reverse order - an EWS item can contain tags for which the
 *  resource doesn't know the Akonadi tag ID. In order to add such a tag to the Akonadi item the
 *  resource needs to fetch the tags in order to learn their ID.
 *
 *  The EwsAkonadiTagSyncJob job can be used to force a synchronization of Akonadi tags to Exchange.
 *
 *  In future once Akonadi tag support is fixed the explicit synchronization will become obsolete.
 */
class EwsTagStore : public QObject
{
    Q_OBJECT
public:
    explicit EwsTagStore(QObject *parent);
    ~EwsTagStore() override;

    /**
     *  @brief  Load the tag information from the supplied server-side list.
     *
     *  Convenience method used to explicitly pass the server-side tag list to the store. This can
     *  be used if the tag information can be retrieved as part of some other operation. This saves
     *  an extra request to the server that would be needed when retrieveGlobalTags() is called.
     */
    bool readTags(const QStringList &taglist, int version);

    /**
     *  @brief  Writes tag-related properties to the EWS item
     *
     *  This method can be used when an Akonadi item is about to be created/updated on the server.
     *  It will take the list of tags and fill the necessary Exchange properties used to store the
     *  tag information on the server.
     */
    bool writeEwsProperties(const Akonadi::Item &item, EwsItem &ewsItem) const;

    /**
     *  @brief  Reads tag information from the EWS item
     *
     *  This method reads the server-side tag information for an item and populates the Akonadi
     *  item with tags.
     *
     *  The code returns @e true if all tags have been converted from Exchange properties into
     *  Akonadi tags. If at least one tag is not found in Akonadi database the method returns
     *  @e false, unless @e ignoreMissing is set to true, in which case the missing tags are
     *  ignored.
     */
    bool readEwsProperties(Akonadi::Item &item, const EwsItem &ewsItem, bool ignoreMissing) const;

    /**
     *  @brief  Checks if a given Akonadi tag is in the store
     */
    bool containsId(Akonadi::Tag::Id id) const;

    /**
     *  @brief  Retrieve the remote identifier of a tag
     *
     *  This identifier can be used to populate the per-item Exchange tag list property.
     */
    QByteArray tagRemoteId(Akonadi::Tag::Id id) const;

    /**
     *  @brief  Retrieve the display name of the tag
     *
     *  This name can be used to populate the Exchange item category list.
     */
    QString tagName(Akonadi::Tag::Id id) const;

    /**
     *  @brief  Add or update a tag in the store.
     *
     *  This method makes the tag known to the tag store. The tag object needs to be populated
     *  with all information that should be stored in Exchange (uid, attributes).
     *
     *  @note   This method does not update the server-side copy of the tag list. This needs to
     *          be done explicitly by starting an EwsGlobalTagsWriteJob.
     */
    void addTag(const Akonadi::Tag &tag);

    /**
     *  @brief  Add or update a list of tags in the store.
     *
     *  This method makes the tags known to the tag store. The tag objects needs to be populated
     *  with all information that should be stored in Exchange (uid, attributes).
     *
     *  @note   This method does not update the server-side copy of the tag list. This needs to
     *          be done explicitly by starting an EwsGlobalTagsWriteJob.
     */
    void addTags(const Akonadi::Tag::List &tags);

    /**
     *  @brief  Remove a tag from the store.
     *
     *  This method removes the tag from the tag store.
     *
     *  @note   This method does not update the server-side copy of the tag list. This needs to
     *          be done explicitly by starting an EwsGlobalTagsWriteJob.
     */
    void removeTag(const Akonadi::Tag &tag);

    /**
     *  @brief  Retrieve the Akonadi tag list
     *
     *  This method retrieves a list of tags as Akonadi tag objects. All objects are fully populated
     *  with any data stored on the server.
     */
    Akonadi::Tag::List tags() const;

    Akonadi::Tag::Id tagIdForRid(const QByteArray &rid) const;

    /**
     *  @brief  Synchronize the store with tag list from Akonadi
     *
     *  This function is an extension to addTags() in that for each tag it checks whether it already
     *  exists in the store and whether it has changed.
     *
     *  The return value indicates if the store was modified during the synchronization. If at least
     *  one new or changed tag was found the return value is TRUE. Otherwise the method returns FALSE.
     *  The return value serves as information as to whether the tags should be pushed to Exchange.
     */
    bool syncTags(const Akonadi::Tag::List &tags);

    /**
     *  @brief  Retrieve a list of serialized tags
     *
     *  Returns a list of tags serialized into strings. This list is intended to be stored on the
     *  Exchange server.
     */
    QStringList serialize() const;

    /**
     *  @brief  Returns the current tag list version
     *
     *  Each time a change is made to the tag list the version number is incremented. This number
     *  can later be used to determine if a tag list sync from Exchange is needed or not.
     */
    int version() const;

private:
    QByteArray serializeTag(const Akonadi::Tag &tag) const;
    bool unserializeTag(const QByteArray &data, Akonadi::Tag &tag) const;

    /** Master tag list (keyed by the tag remote identifier) */
    QHash<QByteArray, QByteArray> mTagData;
    /** Mapping used to determine the tag remote identifier based on Akonadi tag identifier. */
    QHash<Akonadi::Tag::Id, QByteArray> mTagIdMap;
    /** Mapping used to determine the tag display name based on Akonadi tag identifier. */
    QHash<Akonadi::Tag::Id, QString> mTagNameMap;

    int mVersion;
};

#endif
