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

#include "ewsitemshape.h"

void EwsItemShape::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("ItemShape"));

    // Write the base shape
    writeBaseShape(writer);

    // Write the IncludeMimeContent element (if applicable)
    if (mFlags.testFlag(IncludeMimeContent)) {
        writer.writeTextElement(ewsTypeNsUri, QStringLiteral("IncludeMimeContent"), QStringLiteral("true"));
    }

    // Write the BodyType element
    if (mBodyType != BodyNone) {
        QString bodyTypeText;

        switch (mBodyType) {
        case BodyHtml:
            bodyTypeText = QStringLiteral("HTML");
            break;
        case BodyText:
            bodyTypeText = QStringLiteral("Text");
            break;
        default:
        //case BodyBest:
            bodyTypeText = QStringLiteral("Best");
            break;
        }
        writer.writeTextElement(ewsTypeNsUri, QStringLiteral("BodyType"), bodyTypeText);
    }

    // Write the FilterHtmlContent element (if applicable)
    if (mBodyType == BodyHtml && mFlags.testFlag(FilterHtmlContent)) {
        writer.writeTextElement(ewsTypeNsUri, QStringLiteral("FilterHtmlContent"), QStringLiteral("true"));
    }

    // Write the ConvertHtmlCodePageToUTF8 element (if applicable)
    if (mBodyType == BodyHtml && mFlags.testFlag(ConvertHtmlToUtf8)) {
        writer.writeTextElement(ewsTypeNsUri, QStringLiteral("ConvertHtmlCodePageToUTF8"), QStringLiteral("true"));
    }

    // Write properties (if any)
    writeProperties(writer);

    writer.writeEndElement();
}

