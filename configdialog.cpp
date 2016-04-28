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
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#include <KWindowSystem/KWindowSystem>

#include "configdialog.h"
#include "ui_configdialog.h"
#include "settings.h"
#include "ewsresource.h"
#include "ewsautodiscoveryjob.h"
#include "ewsgetfolderrequest.h"
#include "progressdialog.h"
#include "ewssubscriptionwidget.h"

ConfigDialog::ConfigDialog(EwsResource *parentResource, EwsClient &client, WId wId)
    : QDialog(), mParentResource(parentResource), mAutoDiscoveryNeeded(false), mTryConnectNeeded(false),
      mProgressDialog(Q_NULLPTR)
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
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::dialogAccepted);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
    mainLayout->addWidget(mButtonBox);

    setWindowTitle(i18n("Microsoft Exchange Configuration"));

    mUi = new Ui::SetupServerView;
    mUi->setupUi(mainWidget);
    mUi->accountName->setText(parentResource->name());

    mSubWidget = new EwsSubscriptionWidget(client, this);
    mUi->subscriptionTabLayout->addWidget(mSubWidget);

    mConfigManager = new KConfigDialogManager(this, Settings::self());
    mConfigManager->updateWidgets();
    switch (Settings::retrievalMethod()) {
    case 0:
        mUi->pollRadioButton->setChecked(true);
        break;
    case 1:
        mUi->streamingRadioButton->setChecked(true);
        break;
    default:
        break;
    }

    const EwsServerVersion &serverVer = client.serverVersion();
    if (serverVer.isValid()) {
        mUi->serverStatusText->setText(i18nc("Server status", "OK"));
        mUi->serverVersionText->setText(serverVer.toString());
    }

    bool baseUrlEmpty = mUi->kcfg_BaseUrl->text().isEmpty();
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!baseUrlEmpty);
    mUi->tryConnectButton->setEnabled(!baseUrlEmpty);
    mTryConnectNeeded = baseUrlEmpty;

    QString password;
    mParentResource->requestPassword(password, false);
    mUi->passwordEdit->setText(password);

    connect(okButton, &QPushButton::clicked, this, &ConfigDialog::save);
    connect(mUi->autodiscoverButton, &QPushButton::clicked, this, &ConfigDialog::performAutoDiscovery);
    connect(mUi->kcfg_Username, SIGNAL(textChanged(const QString&)), this, SLOT(setAutoDiscoveryNeeded()));
    connect(mUi->passwordEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setAutoDiscoveryNeeded()));
    connect(mUi->kcfg_Domain, SIGNAL(textChanged(const QString&)), this, SLOT(setAutoDiscoveryNeeded()));
    connect(mUi->kcfg_Email, SIGNAL(textChanged(const QString&)), this, SLOT(setAutoDiscoveryNeeded()));
    connect(mUi->kcfg_BaseUrl, SIGNAL(textChanged(const QString&)), this, SLOT(enableTryConnect()));
    connect(mUi->tryConnectButton, &QPushButton::clicked, this, &ConfigDialog::tryConnect);
}

ConfigDialog::~ConfigDialog()
{
    delete mUi;
}

void ConfigDialog::save()
{
    mParentResource->setName(mUi->accountName->text());
    mConfigManager->updateSettings();
    if (mUi->pollRadioButton->isChecked()) {
        Settings::setRetrievalMethod(0);
    }
    else {
        Settings::setRetrievalMethod(1);
    }

    Settings::setServerSubscription(mSubWidget->subscriptionEnabled());
    if (mSubWidget->subscribedListValid()) {
        Settings::setServerSubscriptionList(mSubWidget->subscribedList());
    }

    Settings::self()->save();

    mParentResource->setPassword(mUi->passwordEdit->text());
}

void ConfigDialog::performAutoDiscovery()
{
    mAutoDiscoveryJob = new EwsAutodiscoveryJob(mUi->kcfg_Email->text(),
        fullUsername(), mUi->passwordEdit->text(), this);
    connect(mAutoDiscoveryJob, &EwsAutodiscoveryJob::result, this, &ConfigDialog::autoDiscoveryFinished);
    mProgressDialog = new ProgressDialog(this, ProgressDialog::AutoDiscovery);
    connect(mProgressDialog, &QDialog::rejected, this, &ConfigDialog::autoDiscoveryCancelled);
    mAutoDiscoveryJob->start();
    mProgressDialog->show();
}

void ConfigDialog::autoDiscoveryFinished(KJob *job)
{
    if (job->error() || job != mAutoDiscoveryJob) {
        QMessageBox::critical(this, i18n("Autodiscovery failed"), job->errorText());
        mProgressDialog->reject();
    }
    else {
        mProgressDialog->accept();
        mUi->kcfg_BaseUrl->setText(mAutoDiscoveryJob->ewsUrl());
    }
    mAutoDiscoveryJob->deleteLater();
    mAutoDiscoveryJob = Q_NULLPTR;
    mProgressDialog->deleteLater();
    mProgressDialog = Q_NULLPTR;
}

void ConfigDialog::tryConnectFinished(KJob *job)
{
    if (job->error() || job != mTryConnectJob) {
        QMessageBox::critical(this, i18n("Connection failed"), job->errorText());
        mUi->serverStatusText->setText(i18nc("Server status", "Failed"));
        mUi->serverVersionText->setText(i18n("Unknown"));
        mProgressDialog->reject();
    }
    else {
        mUi->serverStatusText->setText(i18nc("Server status", "OK"));
        mUi->serverVersionText->setText(mTryConnectJob->serverVersion().toString());
        mProgressDialog->accept();
    }
    //mTryConnectJob->deleteLater();
    mTryConnectJob = Q_NULLPTR;
    //mProgressDialog->deleteLater();
    mProgressDialog = Q_NULLPTR;
}

void ConfigDialog::autoDiscoveryCancelled()
{
    if (mAutoDiscoveryJob) {
        mAutoDiscoveryJob->kill();
    }
    mProgressDialog->deleteLater();
    mProgressDialog = Q_NULLPTR;
}

void ConfigDialog::tryConnectCancelled()
{
    if (mTryConnectJob) {
        mTryConnectJob->kill();
    }
    //mTryConnectJob->deleteLater();
    mTryConnectJob = Q_NULLPTR;
}

void ConfigDialog::setAutoDiscoveryNeeded()
{
    mAutoDiscoveryNeeded = true;
    mTryConnectNeeded = true;

    /* Enable the OK button when at least the e-mail and username fields are set. Additionally if
     * autodiscovery is disabled, enable the OK button only if the base URL is set. */
    bool okEnabled = !mUi->kcfg_Username->text().isEmpty() && !mUi->kcfg_Email->text().isEmpty();
    if (!mUi->kcfg_AutoDiscovery->isChecked() && mUi->kcfg_BaseUrl->text().isEmpty()) {
        okEnabled = false;
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

QString ConfigDialog::fullUsername() const
{
    QString username = mUi->kcfg_Username->text();
    QString domain = mUi->kcfg_Domain->text();
    if (!domain.isEmpty()) {
        username.prepend(domain + QStringLiteral("\\"));
    }
    return username;
}

void ConfigDialog::dialogAccepted()
{
    if (mUi->kcfg_AutoDiscovery->isChecked() && mAutoDiscoveryNeeded) {
        mAutoDiscoveryJob = new EwsAutodiscoveryJob(mUi->kcfg_Email->text(),
            fullUsername(), mUi->passwordEdit->text(), this);
        connect(mAutoDiscoveryJob, &EwsAutodiscoveryJob::result, this, &ConfigDialog::autoDiscoveryFinished);
        mProgressDialog = new ProgressDialog(this, ProgressDialog::AutoDiscovery);
        connect(mProgressDialog, &QDialog::rejected, this, &ConfigDialog::autoDiscoveryCancelled);
        mAutoDiscoveryJob->start();
        if (!mProgressDialog->exec()) {
            if (QMessageBox::question(this, i18n("Exchange server autodiscovery"),
                i18n("Autodiscovery failed. This can be caused by incorrect parameters. Do you still want to save your settings?")) == QMessageBox::Yes) {
                accept();
                return;
            }
            else {
                return;
            }
        }
    }

    if (mTryConnectNeeded) {
        EwsClient cli;
        cli.setUrl(mUi->kcfg_BaseUrl->text());
        cli.setCredentials(fullUsername(), mUi->passwordEdit->text());
        mTryConnectJob = new EwsGetFolderRequest(cli, this);
        mTryConnectJob->setFolderShape(EwsShapeIdOnly);
        mTryConnectJob->setFolderIds(EwsId::List() << EwsId(EwsDIdInbox));
        connect(mTryConnectJob, &EwsGetFolderRequest::result, this, &ConfigDialog::tryConnectFinished);
        mProgressDialog = new ProgressDialog(this, ProgressDialog::TryConnect);
        connect(mProgressDialog, &QDialog::rejected, this, &ConfigDialog::tryConnectCancelled);
        mTryConnectJob->start();
        if (!mProgressDialog->exec()) {
            if (QMessageBox::question(this, i18n("Exchange server connection"),
                i18n("Connecting to Exchange failed. This can be caused by incorrect parameters. Do you still want to save your settings?")) == QMessageBox::Yes) {
                accept();
                return;
            }
            else {
                return;
            }
        }
        else {
            accept();
        }
    }

    if (!mTryConnectNeeded && !mAutoDiscoveryNeeded) {
        accept();
    }
}

void ConfigDialog::enableTryConnect()
{
    mTryConnectNeeded = true;
    bool baseUrlEmpty = mUi->kcfg_BaseUrl->text().isEmpty();

    /* Enable the OK button when at least the e-mail and username fields are set. Additionally if
     * autodiscovery is disabled, enable the OK button only if the base URL is set. */
    bool okEnabled = !mUi->kcfg_Username->text().isEmpty() && !mUi->kcfg_Email->text().isEmpty();
    if (!mUi->kcfg_AutoDiscovery->isChecked() && baseUrlEmpty) {
        okEnabled = false;
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
    mUi->tryConnectButton->setEnabled(!baseUrlEmpty);
}

void ConfigDialog::tryConnect()
{
    EwsClient cli;
    cli.setUrl(mUi->kcfg_BaseUrl->text());
    cli.setCredentials(fullUsername(), mUi->passwordEdit->text());
    mTryConnectJob = new EwsGetFolderRequest(cli, this);
    mTryConnectJob->setFolderShape(EwsShapeIdOnly);
    mTryConnectJob->setFolderIds(EwsId::List() << EwsId(EwsDIdInbox));
    mProgressDialog = new ProgressDialog(this, ProgressDialog::TryConnect);
    connect(mProgressDialog, &QDialog::rejected, this, &ConfigDialog::tryConnectCancelled);
    mProgressDialog->show();
    if (!mTryConnectJob->exec()) {
        mUi->serverStatusText->setText(i18nc("Server status", "Failed"));
        mUi->serverVersionText->setText(i18n("Unknown"));
        QMessageBox::critical(this, i18n("Connection failed"), mTryConnectJob->errorText());
    }
    else {
        mUi->serverStatusText->setText(i18nc("Server status", "OK"));
        mUi->serverVersionText->setText(mTryConnectJob->serverVersion().toString());
    }
    mProgressDialog->hide();
}
