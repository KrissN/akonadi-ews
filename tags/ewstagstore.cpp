/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewstagstore.h"

#include <QDataStream>

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/TagAttribute>

#include "ewsclient_debug.h"
#include "ewsitem.h"
#include "ewsresource.h"

using namespace Akonadi;

static Q_CONSTEXPR quint32 TagDataVer1 = 1;
static Q_CONSTEXPR QDataStream::Version TagDataVer1StreamVer = QDataStream::Qt_5_4;

EwsTagStore::EwsTagStore(QObject *parent)
    : QObject(parent), mVersion(0)
{

}

EwsTagStore::~EwsTagStore()
{

}

QByteArray EwsTagStore::serializeTag(const Akonadi::Tag &tag) const
{
    QByteArray tagData;
    QDataStream stream(&tagData, QIODevice::WriteOnly);

    stream.setVersion(TagDataVer1StreamVer);
    stream << TagDataVer1;
    stream << tag.name() << tag.gid();
    Attribute::List attrs = tag.attributes();
    stream << (int)attrs.size();

    Q_FOREACH(const Attribute *attr, attrs) {
        stream << attr->type();
        stream << attr->serialized();
    }

    return tagData;
}

bool EwsTagStore::unserializeTag(const QByteArray &data, Akonadi::Tag &tag) const
{
    QDataStream stream(data);

    quint32 ver;
    stream >> ver;

    QString name;
    QByteArray gid;
    int numAttrs;
    Attribute::List attributes;

    switch (ver) {
    case TagDataVer1:
        stream.setVersion(TagDataVer1StreamVer);
        stream >> name >> gid;
        stream >> numAttrs;
        if (stream.status() != QDataStream::Ok) {
            QStringLiteral("Error reading tag version 1");
            return false;
        }

        for (int i = 0; i < numAttrs; ++i) {
            QByteArray attrType, attrData;
            stream >> attrType >> attrData;
            if (stream.status() != QDataStream::Ok) {
                QStringLiteral("Error reading tag version 1");
                return false;
            }
            Attribute *attr = AttributeFactory::createAttribute(attrType);
            attr->deserialize(data);
            attributes.append(attr);
        }
        break;
    default:
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Unknown tag data version (%1)").arg(ver);
        return false;
    }
    tag.setName(name);
    tag.setGid(gid);

    Q_FOREACH(Attribute *attr, attributes) {
        tag.addAttribute(attr);
    }

    return true;
}

bool EwsTagStore::readTags(const QStringList &taglist, int version)
{

    if (version < mVersion) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Reading tags from older version (have %1, got %2)")
                        .arg(mVersion).arg(version);
        return false;
    } else if (version == mVersion) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Both tag lists in version %1 - not syncing").arg(version);
        return true;
    }

    mTagData.clear();

    Q_FOREACH(const QString &tag, taglist) {
        QByteArray tagdata = qUncompress(QByteArray::fromBase64(tag.toAscii()));
        if (tagdata.isNull()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Incorrect tag data");
        } else {
            QDataStream stream(tagdata);
            stream.setVersion(QDataStream::Qt_5_4);
            QByteArray key, data;
            stream >> key >> data;
            if (stream.status() != QDataStream::Ok) {
                qCDebugNC(EWSRES_LOG) << QStringLiteral("Incorrect tag entry");
                mTagData.clear();
                return false;
            } else {
                mTagData.insert(key, data);
            }
        }
    }

    mVersion = version;

    return true;
}

QStringList EwsTagStore::serialize() const
{
    QStringList tagList;

    for (auto it = mTagData.cbegin(); it != mTagData.cend(); ++it) {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_4);
        stream << it.key();
        stream << it.value();
        tagList.append(qCompress(data, 9).toBase64());
    }

    return tagList;
}

Tag::List EwsTagStore::tags() const
{
    Tag::List tagList;

    for (auto it = mTagData.cbegin(); it != mTagData.cend(); ++it) {
        Tag tag(-1);
        if (unserializeTag(it.value(), tag)) {
            tagList.append(tag);
        }
    }

    return tagList;
}

bool EwsTagStore::containsId(Akonadi::Tag::Id id) const
{
    return mTagIdMap.contains(id);
}

void EwsTagStore::addTag(const Akonadi::Tag &tag)
{
    addTags(Akonadi::Tag::List() << tag);
}

void EwsTagStore::addTags(const Akonadi::Tag::List &tags)
{
    syncTags(tags);
}

bool EwsTagStore::syncTags(const Akonadi::Tag::List &tags)
{
    /* TODO: Remote tag support is partially broken in Akonadi. In particular it is not possible
     * to associate a remote identifier with a tag as the database schema is broken. Therefore
     * the EWS resource relies on the fact that the remote identifier is the same as the local uid
     * of the tag and doesn't set the remote identifier on the tag object. This is possinle as
     * tags don't exist as Exchange objects and are only stored in private properties that are in
     * full control. */

    bool changed = false;

    QList<QByteArray> tagIds = mTagData.keys();
    Q_FOREACH(const Tag &tag, tags) {
        QByteArray serialized = serializeTag(tag);
        auto it = mTagData.find(tag.gid());
        /* First check if the tag exists or if it has been changed. Only once that is done
         * check if the store knows the tag name and Akonadi id. The separation is necessary as
         * the store might have the full list of tags from Exchange, but without Akonadi IDs. When
         * a sync is done it may only yield those IDs without any of the tags changed. In such case
         * the function should return false as no actual change has been made.
         */
        if ((it == mTagData.end()) || (*it != serialized)) {
            mTagData.insert(tag.gid(), serialized);
            changed = true;
        }
        if (it != mTagData.end()) {
        	tagIds.removeOne(tag.gid());
        }
        if (!mTagIdMap.contains(tag.id())) {
            mTagIdMap.insert(tag.id(), tag.gid());
            QString name;
            if (tag.hasAttribute<TagAttribute>()) {
                name = tag.attribute<TagAttribute>()->displayName();
            } else {
                name = tag.name();
            }
            if (!name.isEmpty()) {
                mTagNameMap.insert(tag.id(), tag.name());
            }
        }
    }

    Q_FOREACH(const QByteArray &tagId, tagIds) {
        mTagData.remove(tagId);
    }

    if (changed) {
        ++mVersion;
    }

    return changed;
}

void EwsTagStore::removeTag(const Akonadi::Tag &tag)
{
    QByteArray rid = mTagIdMap.value(tag.id());
    mTagIdMap.remove(tag.id());
    mTagNameMap.remove(tag.id());
    mTagData.remove(rid);

    ++mVersion;
}

QByteArray EwsTagStore::tagRemoteId(Akonadi::Tag::Id id) const
{
    return mTagIdMap.value(id);
}

QString EwsTagStore::tagName(Akonadi::Tag::Id id) const
{
    return mTagNameMap.value(id);
}

Tag::Id EwsTagStore::tagIdForRid(const QByteArray &rid) const
{
    return mTagIdMap.key(rid, -1);
}

bool EwsTagStore::readEwsProperties(Akonadi::Item &item, const EwsItem &ewsItem, bool ignoreMissing) const
{
    QVariant tagProp = ewsItem[EwsResource::tagsProperty];
    if (tagProp.isValid() && tagProp.canConvert<QStringList>()) {
        QStringList tagRids = tagProp.toStringList();
        Tag::List tags;
        Q_FOREACH(const QString &tagRid, tagRids) {
            Tag::Id tagId = tagIdForRid(tagRid.toAscii());
            if (tagId == -1) {
                /* Tag not found. */
                qCDebug(EWSRES_LOG) << QStringLiteral("Found missing tag: %1").arg(tagRid);
                if (ignoreMissing) {
                    continue;
                } else {
                    return false;
                }
            }
            Tag tag(tagId);
            item.setTag(tag);
        }
    }

    return true;
}

bool EwsTagStore::writeEwsProperties(const Akonadi::Item &item, EwsItem &ewsItem) const
{
    if (!item.tags().isEmpty()) {
        QStringList tagList;
        QStringList categoryList;
        Q_FOREACH(const Tag &tag, item.tags()) {
            if (!containsId(tag.id())) {
                return false;
            }
            tagList.append(tagRemoteId(tag.id()));
            QString name = tagName(tag.id());
            if (!name.isEmpty()) {
                categoryList.append(name);
            }
        }
        ewsItem.setProperty(EwsResource::tagsProperty, tagList);
        ewsItem.setField(EwsItemFieldCategories, categoryList);
    }

    return true;
}

int EwsTagStore::version() const
{
    return mVersion;
}
