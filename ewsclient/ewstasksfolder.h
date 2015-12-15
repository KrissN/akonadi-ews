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

#ifndef EWSTASKSFOLDER_H
#define EWSTASKSFOLDER_H

#include "ewsfolderbase.h"

class EwsTasksFolderPrivate;

class EwsTasksFolder : public EwsFolderBase
{
    Q_OBJECT
public:
    EwsTasksFolder(EwsFolderId id, EwsClient *parent);
    EwsTasksFolder(QXmlStreamReader &reader, EwsClient *parent);
    virtual ~EwsTasksFolder();
private:
    EwsTasksFolder(QSharedDataPointer<EwsFolderBasePrivate> priv, EwsClient *parent);

    friend class EwsFolderBase;
};

#endif

