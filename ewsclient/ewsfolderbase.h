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

#ifndef EWSFOLDERBASE_H
#define EWSFOLDERBASE_H

#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>

#include "ewsfolderid.h"

class EwsFolderBasePrivate;
class EwsClient;
class EwsGetFolderRequest;
class EwsFolderItemBase;

class EwsMailFolder;

class EwsFolderBase : public QObject
{
    Q_OBJECT
public:
    EwsFolderBase(EwsFolderId id, EwsClient *parent);
    EwsFolderBase(const EwsFolderBase &other);
    EwsFolderBase(EwsFolderBase &&other);
    virtual ~EwsFolderBase();
    EwsFolderBase& operator=(const EwsFolderBase &other);

    void setShape(EwsBaseShape shape);

    bool update();

    bool isValid() const;

    EwsFolderType type() const;
    EwsFolderId id() const;
    EwsFolderId parentId() const;
    QString folderClass() const;
    QString displayName() const;
    unsigned totalCount() const;
    unsigned childFolderCount() const;

    EwsMailFolder* toMailFolder();

protected Q_SLOTS:
    void requestFinished();
protected:
    EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsFolderId id, EwsClient *parent);
    EwsFolderBase(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsClient *parent);

    void resetFields();
    bool readBaseFolderElement(QXmlStreamReader &reader);

    QSharedDataPointer<EwsFolderBasePrivate> d;
};

#endif

