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

#include "settings.h"

#include <QPointer>

#include <KWallet/KWallet>
#include <KWidgetsAddons/KPasswordDialog>
#include <KI18n/KLocalizedString>

Settings::Settings(WId windowId)
    : SettingsBase(), mWindowId(windowId)
{
}

Settings::~Settings()
{
}

bool Settings::requestPassword(QString &password, bool ask)
{
    bool status = true;

    if (!mPassword.isEmpty()) {
        password = mPassword;
        return true;
    }

    QScopedPointer<KWallet::Wallet> wallet(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                                       mWindowId));
    if (wallet && wallet->isOpen()) {
        if (wallet->hasFolder(QStringLiteral("akonadi-ews"))) {
            wallet->setFolder(QStringLiteral("akonadi-ews"));
            wallet->readPassword(config()->name(), password);
        } else {
            wallet->createFolder(QStringLiteral("akonadi-ews"));
        }
    } else {
        status = false;
    }

    if (!status) {
        if (!ask) {
            return false;
        }

        QPointer<KPasswordDialog> dlg = new KPasswordDialog(nullptr);
        dlg->setModal(true);
        dlg->setPrompt(i18n("Please enter password for user '%1' and Exchange account '%2'.",
                            username(), email()));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        if (dlg->exec() == QDialog::Accepted) {
            password = dlg->password();
            setPassword(password);
        } else {
            delete dlg;
            return false;
        }
        delete dlg;
    }

    return true;
}

void Settings::setPassword(const QString &password)
{
    mPassword = password;
    QScopedPointer<KWallet::Wallet> wallet(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                                       mWindowId));
    if (wallet && wallet->isOpen()) {
        if (!wallet->hasFolder(QStringLiteral("akonadi-ews"))) {
            wallet->createFolder(QStringLiteral("akonadi-ews"));
        }
        wallet->setFolder(QStringLiteral("akonadi-ews"));
        wallet->writePassword(config()->name(), password);
    }
}

void Settings::setTestPassword(const QString &password)
{
    mPassword = password;
}

