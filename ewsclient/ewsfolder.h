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

#ifndef EWSFOLDER_H
#define EWSFOLDER_H

#include "ewsitembase.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class EwsFolderPrivate;

class EwsFolder : public EwsItemBase
{
public:
    typedef QList<EwsFolder> List;

    EwsFolder();
    explicit EwsFolder(QXmlStreamReader &reader);
    EwsFolder(const EwsFolder &other);
    EwsFolder(EwsFolder &&other);
    virtual ~EwsFolder();

    EwsFolder& operator=(const EwsFolder &other);
    EwsFolder& operator=(EwsFolder &&other);

    EwsFolderType type() const;
    void setType(EwsFolderType type);

    const QVector<EwsFolder> childFolders() const;
    void addChild(EwsFolder &child);
    EwsFolder* parentFolder() const;
    void setParentFolder(EwsFolder *parent);

    bool write(QXmlStreamWriter &writer) const;
protected:
    bool readBaseFolderElement(QXmlStreamReader &reader);
};

Q_DECLARE_METATYPE(EwsFolder)

#endif
