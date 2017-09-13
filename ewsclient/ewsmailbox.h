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

#ifndef EWSMAILBOX_H
#define EWSMAILBOX_H

#include <QSharedDataPointer>
#include <QXmlStreamReader>

#include "ewsitembase.h"

class EwsMailboxPrivate;

namespace KMime
{
namespace Types
{
class Mailbox;
}
}

class EwsMailbox
{
public:
    typedef QList<EwsMailbox> List;

    EwsMailbox();
    explicit EwsMailbox(QXmlStreamReader &reader);
    EwsMailbox(const EwsMailbox &other);
    EwsMailbox(EwsMailbox &&other);
    virtual ~EwsMailbox();

    EwsMailbox &operator=(const EwsMailbox &other);
    EwsMailbox &operator=(EwsMailbox &&other);

    bool isValid() const;
    QString name() const;
    QString email() const;
    QString emailWithName() const;
    operator KMime::Types::Mailbox() const;
protected:
    QSharedDataPointer<EwsMailboxPrivate> d;
};

Q_DECLARE_METATYPE(EwsMailbox)
Q_DECLARE_METATYPE(EwsMailbox::List)

#endif
