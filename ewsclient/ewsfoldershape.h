/*
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

#ifndef EWSFOLDERSHAPE_H
#define EWSFOLDERSHAPE_H

#include <QVector>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewspropertyfield.h"
#include "ewstypes.h"

class EwsFolderShape
{
public:
    explicit EwsFolderShape(EwsBaseShape shape = EwsShapeDefault) : mBaseShape(shape) {};
    ~EwsFolderShape() {};
    EwsFolderShape(const EwsFolderShape &other)
        : mBaseShape(other.mBaseShape), mProps(other.mProps) {};
    EwsFolderShape(EwsFolderShape &&other)
        : mBaseShape(other.mBaseShape), mProps(other.mProps) {};
    EwsFolderShape &operator=(EwsFolderShape &&other) {
        mBaseShape = other.mBaseShape;
        mProps = std::move(other.mProps);
        return *this;
    }
    EwsFolderShape &operator=(const EwsFolderShape &other) {
        mBaseShape = other.mBaseShape;
        mProps = other.mProps;
        return *this;
    }

    void write(QXmlStreamWriter &writer) const;

    friend EwsFolderShape &operator<<(EwsFolderShape &shape, const EwsPropertyField &prop);
protected:
    void writeBaseShape(QXmlStreamWriter &writer) const;
    void writeProperties(QXmlStreamWriter &writer) const;

    EwsBaseShape mBaseShape;
    QVector<EwsPropertyField> mProps;
};

inline EwsFolderShape &operator<<(EwsFolderShape &shape, const EwsPropertyField &prop)
{
    shape.mProps.append(prop);
    return shape;
}

#endif
