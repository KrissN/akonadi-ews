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

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>

#include <KWindowSystem/KWindowSystem>

#include "configdialog.h"
#include "ui_configdialog.h"
#include "settings.h"
#include "ewsresource.h"

ConfigDialog::ConfigDialog(EwsResource *parentResource, WId wId)
    : QDialog(), mParentResource(parentResource)
{
    if (wId) {
        KWindowSystem::setMainWindow(this, wId);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
    mainLayout->addWidget(buttonBox);

    setWindowTitle(i18n("Microsoft Exchange Configuration"));

    Ui::SetupServerView ui;
    ui.setupUi(mainWidget);
    mAccountName = ui.accountName;
    mAccountName->setText(parentResource->name());

    mPollRadioButton = ui.pollRadioButton;
    mStreamingRadioButton = ui.streamingRadioButton;

    mConfigManager = new KConfigDialogManager(this, Settings::self());
    mConfigManager->updateWidgets();
    switch (Settings::retrievalMethod()) {
    case 0:
        mPollRadioButton->setChecked(true);
        break;
    case 1:
        mStreamingRadioButton->setChecked(true);
        break;
    default:
        break;
    }

    connect(okButton, &QPushButton::clicked, this, &ConfigDialog::save);
}

void ConfigDialog::save()
{
    mParentResource->setName(mAccountName->text());
    mConfigManager->updateSettings();
    if (mPollRadioButton->isChecked()) {
        Settings::setRetrievalMethod(0);
    }
    else {
        Settings::setRetrievalMethod(1);
    }
    Settings::self()->save();
}

