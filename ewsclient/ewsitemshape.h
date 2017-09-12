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

#ifndef EWSITEMSHAPE_H
#define EWSITEMSHAPE_H

#include <QFlags>

#include "ewsfoldershape.h"

class EwsItemShape : public EwsFolderShape
{
public:
    enum Flag {
        IncludeMimeContent = 0x01,
        FilterHtmlContent = 0x02,
        ConvertHtmlToUtf8 = 0x04
    };
    enum BodyType {
        BodyNone,
        BodyBest,
        BodyHtml,
        BodyText
    };

    Q_DECLARE_FLAGS(Flags, Flag)

    explicit EwsItemShape(EwsBaseShape shape = EwsShapeDefault) : EwsFolderShape(shape), mBodyType(BodyNone) {};
    EwsItemShape(const EwsItemShape &other)
        : EwsFolderShape(other), mBodyType(BodyNone) {};
    explicit EwsItemShape(EwsFolderShape &&other)
        : EwsFolderShape(other), mBodyType(BodyNone) {};
    EwsItemShape &operator=(EwsItemShape &&other) {
        mBaseShape = other.mBaseShape;
        mProps = std::move(other.mProps);
        mFlags = other.mFlags;
        mBodyType = other.mBodyType;
        return *this;
    }
    EwsItemShape &operator=(const EwsItemShape &other) {
        mBaseShape = other.mBaseShape;
        mProps = other.mProps;
        mFlags = other.mFlags;
        mBodyType = other.mBodyType;
        return *this;
    }

    void setFlags(Flags flags) {
        mFlags = flags;
    }

    void setBodyType(BodyType type) {
        mBodyType = type;
    }

    void write(QXmlStreamWriter &writer) const;
protected:
    Flags mFlags;
    BodyType mBodyType;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(EwsItemShape::Flags)

#endif
