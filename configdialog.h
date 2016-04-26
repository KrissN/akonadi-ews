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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtWidgets/QDialog>

#include <KConfigWidgets/KConfigDialogManager>

class QDialogButtonBox;
class EwsResource;
class EwsClient;
namespace Ui {
class SetupServerView;
}
class KJob;
class EwsAutodiscoveryJob;
class EwsGetFolderRequest;
class ProgressDialog;
class EwsSubscriptionWidget;

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigDialog(EwsResource *parentResource, EwsClient &client, WId windowId);
    virtual ~ConfigDialog();
private Q_SLOTS:
    void save();
    void autoDiscoveryFinished(KJob *job);
    void tryConnectFinished(KJob *job);
    void performAutoDiscovery();
    void autoDiscoveryCancelled();
    void tryConnectCancelled();
    void setAutoDiscoveryNeeded();
    void dialogAccepted();
    void enableTryConnect();
    void tryConnect();
private:
    QString fullUsername() const;

    EwsResource *mParentResource;
    KConfigDialogManager *mConfigManager;
    Ui::SetupServerView *mUi;

    QDialogButtonBox *mButtonBox;
    EwsAutodiscoveryJob *mAutoDiscoveryJob;
    EwsGetFolderRequest *mTryConnectJob;
    bool mAutoDiscoveryNeeded;
    bool mTryConnectNeeded;
    ProgressDialog *mProgressDialog;
    EwsSubscriptionWidget *mSubWidget;
};

#endif
