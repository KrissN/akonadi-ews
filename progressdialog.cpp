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

#include "progressdialog.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

#include <KI18n/KLocalizedString>

ProgressDialog::ProgressDialog(QWidget *parent, Type type)
    : QDialog(parent)
{
    setModal(true);
    QLabel *statusLabel = new QLabel(this);

    QProgressBar *progress = new QProgressBar(this);
    progress->setMaximum(0);

    QVBoxLayout *vLayout = new QVBoxLayout(this);

    vLayout->setMargin(0);

    vLayout->addWidget(statusLabel);
    vLayout->addWidget(progress);

    QPushButton *cancelButton = new QPushButton(this);
    cancelButton->setText(i18n("Cancel"));

    QWidget *progressContainer = new QWidget(this);
    progressContainer->setLayout(vLayout);

    QHBoxLayout *hLayout = new QHBoxLayout(this);

    hLayout->addWidget(progressContainer);
    hLayout->addWidget(cancelButton);

    setLayout(hLayout);
    hLayout->setSizeConstraint(QLayout::SetFixedSize);

    switch (type) {
    case AutoDiscovery:
        setWindowTitle(i18n("Exchange server autodiscovery"));
        statusLabel->setText(i18n("Performing Microsoft Exchange server autodiscovery..."));
        break;
    case TryConnect:
        setWindowTitle(i18n("Connecting to Exchange"));
        statusLabel->setText(i18n("Connecting to Microsoft Exchange server..."));
        break;
    }

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

ProgressDialog::~ProgressDialog()
{
}
