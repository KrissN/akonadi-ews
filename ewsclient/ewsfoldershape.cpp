/*  This file is part of Akonadi EWS Resource
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

#include "ewsfoldershape.h"

static const QString shapeNames[] = {
    QStringLiteral("IdOnly"),
    QStringLiteral("Default"),
    QStringLiteral("AllProperties")
};

void EwsFolderShape::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("FolderShape"));

    // Write the base shape
    writeBaseShape(writer);

    // Write properties (if any)
    writeProperties(writer);

    writer.writeEndElement();
}

void EwsFolderShape::writeBaseShape(QXmlStreamWriter &writer) const
{
    writer.writeTextElement(ewsTypeNsUri, QStringLiteral("BaseShape"), shapeNames[mBaseShape]);
}

void EwsFolderShape::writeProperties(QXmlStreamWriter &writer) const
{
    if (!mProps.isEmpty()) {
        writer.writeStartElement(ewsTypeNsUri, QStringLiteral("AdditionalProperties"));

        foreach (const EwsPropertyField &prop, mProps) {
            prop.write(writer);
        }

        writer.writeEndElement();
    }
}
