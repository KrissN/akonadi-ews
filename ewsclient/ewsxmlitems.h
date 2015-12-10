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

#include "ewstypes.h"
#include "ewsfolderid.h"

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
    EwsBaseShapeItem(EwsFolderShape shape = EwsShapeDefault);
    virtual ~EwsBaseShapeItem();

    void setShape(EwsFolderShape shape) { mShape = shape; };
    EwsFolderShape shape() const { return mShape; };

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsFolderShape mShape;
};

class EwsFolderShapeItem : public EwsXmlItemBase
{
public:
    EwsFolderShapeItem();
    virtual ~EwsFolderShapeItem();

    EwsBaseShapeItem* baseShape() const { return mBaseShape; };
    void setBaseShape(EwsBaseShapeItem *baseShape);
    void setBaseShape(EwsFolderShape shape);

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsBaseShapeItem *mBaseShape;
};

class EwsFolderIdItem : public EwsXmlItemBase
{
public:
    EwsFolderIdItem();
    EwsFolderIdItem(QString id, QString changeKey);
    virtual ~EwsFolderIdItem();

    QString id() const { return mId; };
    void setId(QString id) { mId = id; };
    QString changeKey() const { return mChangeKey; };
    void setChangeKey(QString changeKey) { mChangeKey = changeKey; };

    virtual void write(QXmlStreamWriter &writer) const;
    virtual bool read(QXmlStreamReader &reader);
private:
    QString mId;
    QString mChangeKey;
};

class EwsDistinguishedFolderIdItem : public EwsXmlItemBase
{
public:
    EwsDistinguishedFolderIdItem(EwsDistinguishedId id);
    virtual ~EwsDistinguishedFolderIdItem();

    EwsDistinguishedId id() const { return mId; };
    void setId(EwsDistinguishedId id) { mId = id; };

    virtual void write(QXmlStreamWriter &writer) const;
private:
    EwsDistinguishedId mId;
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
    void setDistinguishedFolderId(EwsDistinguishedId id);

    void setId(const EwsFolderId &id);

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

class EwsResponseMessageBase : public EwsXmlItemBase
{
public:
    EwsResponseMessageBase();
    virtual ~EwsResponseMessageBase();

    EwsResponseClass responseClass() const { return mResponseClass; };

    QString responseCode() const { return mResponseCode; };
    QString responseMessage() const { return mMessageText; };
protected:
    bool readResponseAttr(const QXmlStreamAttributes &attrs);
    bool readResponseElement(QXmlStreamReader &reader);
private:
    EwsResponseClass mResponseClass;
    QString mResponseCode;
    QString mMessageText;
};

class EwsFolderItemBase : public EwsXmlItemBase
{
public:
    EwsFolderItemBase();
    virtual ~EwsFolderItemBase();

    virtual EwsFolderType type() const = 0;

    const EwsFolderIdItem* folderId() const { return mFolderId; };
    const EwsFolderIdItem* parentFolderId() const { return mParentFolderId; };
    QString folderClass() const { return mFolderClass; };
    QString displayName() const { return mDisplayName; };
    int totalCount() const { return mTotalCount; };
    int childFolderCount() const { return mChildFolderCount; };
protected:
    bool readFolderElement(QXmlStreamReader &reader);
private:
    EwsFolderIdItem *mFolderId;
    EwsFolderIdItem *mParentFolderId;
    QString mFolderClass;
    QString mDisplayName;
    int mTotalCount;
    int mChildFolderCount;
    // mExtendedProperties;
    // mManagedFolderInformation;
    // mEffectiveRights;
};

class EwsFolderItem : public EwsFolderItemBase
{
public:
    EwsFolderItem();
    virtual ~EwsFolderItem();

    virtual bool read(QXmlStreamReader &reader);

    virtual EwsFolderType type() const { return EwsFolder; };

    int unreadCount() const { return mUnreadCount; };
private:
    // mPermissionSet;
    int mUnreadCount;
};

class EwsFoldersItem : public EwsXmlItemBase
{
public:
    EwsFoldersItem();
    virtual ~EwsFoldersItem();

    virtual bool read(QXmlStreamReader &reader);
private:
    QList<EwsFolderItemBase*> mFolders;
};

class EwsGetFolderResponseMessageItem : public EwsResponseMessageBase
{
public:
    EwsGetFolderResponseMessageItem();
    virtual ~EwsGetFolderResponseMessageItem();

    const EwsFoldersItem* folders() const { return mFolders; };

    virtual bool read(QXmlStreamReader &reader);
private:
    EwsFoldersItem *mFolders;
};


#endif
