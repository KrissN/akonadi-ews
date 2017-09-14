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

#include "mtaconfigdialog.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>

#include <KWindowSystem/KWindowSystem>

#include <AkonadiCore/AgentFilterProxyModel>
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentInstanceModel>
#include <AkonadiWidgets/AgentInstanceWidget>

#include "ewsmtaresource.h"
#include "mtasettings.h"
#include "ui_mtaconfigdialog.h"

MtaConfigDialog::MtaConfigDialog(EwsMtaResource *parentResource, WId wId)
    : QDialog(), mParentResource(parentResource)
{
    if (wId) {
        KWindowSystem::setMainWindow(this, wId);
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &MtaConfigDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &MtaConfigDialog::reject);
    mainLayout->addWidget(mButtonBox);

    setWindowTitle(i18n("Microsoft Exchange Mail Transport Configuration"));

    mUi = new Ui::SetupServerView;
    mUi->setupUi(mainWidget);
    mUi->accountName->setText(parentResource->name());

    Akonadi::AgentFilterProxyModel *model = mUi->resourceWidget->agentFilterProxyModel();
    model->addCapabilityFilter(QStringLiteral("X-EwsMailTransport"));
    mUi->resourceWidget->view()->setSelectionMode(QAbstractItemView::SingleSelection);

    for (int i = 0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        QVariant v = model->data(index, Akonadi::AgentInstanceModel::InstanceIdentifierRole);
        if (v.toString() == MtaSettings::ewsResource()) {
            mUi->resourceWidget->view()->setCurrentIndex(index);
        }
    }

    connect(okButton, &QPushButton::clicked, this, &MtaConfigDialog::save);
}

MtaConfigDialog::~MtaConfigDialog()
{
    delete mUi;
}

void MtaConfigDialog::save()
{
    MtaSettings::setEwsResource(mUi->resourceWidget->selectedAgentInstances().first().identifier());
    mParentResource->setName(mUi->accountName->text());
    MtaSettings::self()->save();
}

void MtaConfigDialog::dialogAccepted()
{
}

