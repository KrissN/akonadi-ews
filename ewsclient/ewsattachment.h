/*  This file is part of Akonadi EWS Resource
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef EWSATTACHMENT_H
#define EWSATTACHMENT_H

#include <QtCore/QDateTime>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVector>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "ewsitem.h"
#include "ewstypes.h"
#include "ewspropertyfield.h"

class EwsAttachmentPrivate;

class EwsAttachment
{
public:
    enum Type {
        UnknownAttachment,
        ItemAttachment,
        FileAttachment,
        ReferenceAttachment     // Occurs in 2016 XSD, but not documented on MSDN
    };

    typedef QVector<EwsAttachment> List;

    EwsAttachment();
    ~EwsAttachment();
    EwsAttachment(QXmlStreamReader &reader);
    EwsAttachment(const EwsAttachment &other);
    EwsAttachment(EwsAttachment &&other);
    EwsAttachment& operator=(EwsAttachment &&other);
    EwsAttachment& operator=(const EwsAttachment &other);

    bool isValid() const;

    Type type() const;
    void setType(Type type);

    QString id() const;
    void setId(const QString &id);
    void resetId();
    bool hasId() const;

    QString name() const;
    void setName(const QString &name);
    void resetName();
    bool hasName() const;

    QString contentType() const;
    void setContentType(const QString &contentType);
    void resetContentType();
    bool hasContentType() const;

    QString contentId() const;
    void setContentId(const QString &contentId);
    void resetContentId();
    bool hasContentId() const;

    QString contentLocation() const;
    void setContentLocation(const QString &contentLocation);
    void resetContentLocation();
    bool hasContentLocation() const;

    long size() const;
    void setSize(long size);
    void resetSize();
    bool hasSize() const;

    QDateTime lastModifiedTime() const;
    void setLastModifiedTime(const QDateTime &time);
    void resetLastModifiedTime();
    bool hasLastModifiedTime() const;

    bool isInline() const;
    void setIsInline(bool isInline);
    void resetIsInline();
    bool hasIsInline() const;

    bool isContactPhoto() const;
    void setIsContactPhoto(bool isContactPhoto);
    void resetIsContactPhoto();
    bool hasIsContactPhoto() const;

    QByteArray content() const;
    void setContent(const QByteArray &content);
    void resetContent();
    bool hasContent() const;

    const EwsItem &item() const;
    void setItem(const EwsItem &item);
    void resetItem();
    bool hasItem() const;

    void write(QXmlStreamWriter &writer) const;
protected:
    QSharedDataPointer<EwsAttachmentPrivate> d;
};

Q_DECLARE_METATYPE(EwsAttachment)
Q_DECLARE_METATYPE(EwsAttachment::List)

#endif
