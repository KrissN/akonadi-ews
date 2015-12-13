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

#ifndef EWSFOLDERSHAPE_H
#define EWSFOLDERSHAPE_H

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QVector>

#include "ewstypes.h"
#include "ewspropertyfield.h"

class EwsFolderShape
{
public:
    EwsFolderShape(EwsBaseShape shape = EwsShapeDefault) : mBaseShape(shape) {};
    ~EwsFolderShape() {};
    EwsFolderShape(const EwsFolderShape &other)
        : mBaseShape(other.mBaseShape), mProps(other.mProps) {};
#ifdef Q_COMPILER_RVALUE_REFS
    EwsFolderShape(EwsFolderShape &&other)
        : mBaseShape(other.mBaseShape), mProps(other.mProps) {};
    EwsFolderShape& operator=(EwsFolderShape &&other) {
        mBaseShape = other.mBaseShape;
        mProps = std::move(other.mProps);
    }
#endif
    EwsFolderShape& operator=(const EwsFolderShape &other) {
        mBaseShape = other.mBaseShape;
        mProps = other.mProps;
    }

    void write(QXmlStreamWriter &writer) const;

    friend EwsFolderShape &operator<<(EwsFolderShape &shape, const EwsPropertyField &prop);
private:
    EwsBaseShape mBaseShape;
    QVector<EwsPropertyField> mProps;
};

inline EwsFolderShape& operator<<(EwsFolderShape &shape, const EwsPropertyField &prop)
{
    shape.mProps.append(prop);
    return shape;
};

#endif
