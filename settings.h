/*
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "settingsbase.h"

class Settings : public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Ews.Wallet")
public:
    explicit Settings(WId windowId);
    ~Settings() override;

    bool requestPassword(QString &password, bool ask);
public Q_SLOTS:
    Q_SCRIPTABLE void setPassword(const QString &password);
    Q_SCRIPTABLE void setTestPassword(const QString &password);

private:
    WId mWindowId;
    QString mPassword;
};

#endif

