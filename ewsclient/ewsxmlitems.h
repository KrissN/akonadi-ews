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

#ifndef EWSXMLITEMS_H
#define EWSXMLITEMS_H

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

class EwsXmlItemBase
{
public:
    EwsXmlItemBase();
    virtual ~EwsXmlItemBase();

    static const QString soapEnvNsUri;
    static const QString ewsMsgNsUri;
    static const QString ewsTypeNsUri;

    virtual void write(QXmlStreamWriter &writer) const;
    virtual bool read(QXmlStreamReader &reader);
};

class EwsBaseShapeItem : public EwsXmlItemBase
{
public:
    enum Shape {
        IdOnly = 0,
        Default,
        AllProperties
    };

    EwsBaseShapeItem(Shape shape = Default);
    virtual ~EwsBaseShapeItem();

    void setShape(Shape shape) { mShape = shape; };
    Shape shape() const { return mShape; };

    virtual void write(QXmlStreamWriter &writer) const;
private:
    Shape mShape;
};

class EwsFolderShapeItem : public EwsXmlItemBase
{
public:
    EwsFolderShapeItem();
    virtual ~EwsFolderShapeItem();

    EwsBaseShapeItem* baseShape() const { return mBaseShape; };
    void setBaseShape(EwsBaseShapeItem *baseShape);
    void setBaseShape(EwsBaseShapeItem::Shape shape);

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsBaseShapeItem *mBaseShape;
};

class EwsFolderIdItem : public EwsXmlItemBase
{
public:
    EwsFolderIdItem(QString id, QString changeKey);
    virtual ~EwsFolderIdItem();

    QString id() const { return mId; };
    void setId(QString id) { mId = id; };
    QString changeKey() const { return mChangeKey; };
    void setChangeKey(QString changeKey) { mChangeKey = changeKey; };

    virtual void write(QXmlStreamWriter &writer) const;
private:
    QString mId;
    QString mChangeKey;
};

class EwsDistinguishedFolderIdItem : public EwsXmlItemBase
{
public:
    enum DistinguishedId {
        Calendar = 0,
        Contacts,
        DeletedItems,
        Drafts,
        Inbox,
        Journal,
        Notes,
        Outbox,
        SentItems,
        Tasks,
        MsgFolderRoot,
        Root,
        JunkEmail,
        SearchFolders,
        VoiceMail,
        RecoverableItemsRoot,
        RecoverableItemsDeletions,
        RecoverableItemsVersions,
        RecoverableItemsPurges,
        ArchiveRoot,
        ArchiveMsgFolderRoot,
        ArchiveDeletedItems,
        ArchiveRecoverableItemsRoot,
        ArchiveRecoverableItemsDeletions,
        ArchiveRecoverableItemsVersions,
        ArchiveRecoverableItemsPurges
    };
    EwsDistinguishedFolderIdItem(DistinguishedId id);
    virtual ~EwsDistinguishedFolderIdItem();

    DistinguishedId id() const { return mId; };
    void setId(DistinguishedId id) { mId = id; };

    virtual void write(QXmlStreamWriter &writer) const;
private:
    DistinguishedId mId;
};

class EwsFolderIdsItem : public EwsXmlItemBase
{
public:
    EwsFolderIdsItem();
    virtual ~EwsFolderIdsItem();

    EwsFolderIdItem* folderId() const { return mFolderId; };
    void setFolderId(EwsFolderIdItem *folderId);
    void setFolderId(QString id, QString changeKey);

    EwsDistinguishedFolderIdItem* distinguishedFolderId() const { return mDistinguishedFolderId; };
    void setDistinguishedFolderId(EwsDistinguishedFolderIdItem *distinguishedFolderId);
    void setDistinguishedFolderId(EwsDistinguishedFolderIdItem::DistinguishedId id);

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsFolderIdItem *mFolderId;
    EwsDistinguishedFolderIdItem *mDistinguishedFolderId;
};

class EwsGetFolderItem : public EwsXmlItemBase
{
public:
    EwsGetFolderItem();
    virtual ~EwsGetFolderItem();

    EwsFolderIdsItem* folderIds() const { return mFolderIds; };
    void setFolderIds(EwsFolderIdsItem *folderIds);

    EwsFolderShapeItem* folderShape() const { return mFolderShape; };
    void setFolderShape(EwsFolderShapeItem *folderShape);

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsFolderIdsItem *mFolderIds;
    EwsFolderShapeItem *mFolderShape;
};

class EwsResponseMessageItem : public EwsXmlItemBase
{
public:
    enum ResponseClass {
        Success = 0,
        Warning,
        Error
    };

    EwsResponseMessageItem();
    virtual ~EwsResponseMessageItem();

    ResponseClass responseClass() const { return mResponseClass; };

protected:
    bool readResponseAttr(const QXmlStreamAttributes &attrs);
    bool readResponseElement(QXmlStreamReader &reader);
private:
    ResponseClass mResponseClass;
    QString mResponseCode;
    QString mMessageText;
};

class EwsGetFolderResponseMessageItem : public EwsResponseMessageItem
{
public:
    EwsGetFolderResponseMessageItem();
    virtual ~EwsGetFolderResponseMessageItem();

    virtual bool read(QXmlStreamReader &reader);
private:
};



#endif
