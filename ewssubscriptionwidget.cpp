/*  This file is part of Akonadi EWS Resource
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

#include "ewssubscriptionwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <KItemModels/KRecursiveFilterProxyModel>
#include <KLocalizedString>
#include <KWidgetsAddons/KMessageWidget>

#include "ewsclient.h"
#include "ewsfindfolderrequest.h"
#include "ewssubscribedfoldersjob.h"
#include "settings.h"

class EwsSubscriptionFilterModel : public KRecursiveFilterProxyModel
{
    Q_OBJECT
public:
    EwsSubscriptionFilterModel(QObject *parent = 0);
    ~EwsSubscriptionFilterModel();
public Q_SLOTS:
    void setFilterSelected(bool enabled);
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
private:
    bool hasCheckedChildren(QModelIndex index) const;
    bool mFilterSelected;
};

class EwsSubscriptionWidgetPrivate : public QObject
{
    Q_OBJECT
public:
    EwsSubscriptionWidgetPrivate(EwsClient &client, Settings *settings, QObject *parent);
    ~EwsSubscriptionWidgetPrivate();

    enum TreeModelRoles {
        ItemIdRole = Qt::UserRole + 1
    };

    void populateFolderTree();

public Q_SLOTS:
    void enableCheckBoxToggled(bool checked);
    void reloadFolderList(bool);
    void resetSelection(bool);
    void readFolderListFinished(KJob *job);
    void subscribedFoldersJobFinished(KJob *job);
    void treeItemChanged(QStandardItem *item);
    void filterTextChanged(const QString &text);
public:
    bool mEnabled;
    QCheckBox *mEnableCheckBox;
    QTreeView *mFolderTreeView;
    QWidget *mSubContainer;
    QPushButton *mRefreshButton;
    EwsClient &mClient;
    KMessageWidget *mMsgWidget;
    QStandardItemModel *mFolderTreeModel;
    QHash<QString, QStandardItem*> mFolderItemHash;
    int mFolderListPendingRequests;
    EwsFolder::List mFolders;
    EwsId::List mSubscribedIds;
    EwsId::List mOrigSubscribedIds;
    bool mSubscribedIdsRetrieved;
    EwsSubscriptionFilterModel *mFilterModel;
    Settings *mSettings;

    EwsSubscriptionWidget *q_ptr;
    Q_DECLARE_PUBLIC(EwsSubscriptionWidget)
};

EwsSubscriptionFilterModel::EwsSubscriptionFilterModel(QObject *parent)
    : KRecursiveFilterProxyModel(parent), mFilterSelected(false)
{
}

EwsSubscriptionFilterModel::~EwsSubscriptionFilterModel()
{
}

bool EwsSubscriptionFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    bool show = true;
    if (mFilterSelected) {
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);

        show = sourceIndex.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        show |= hasCheckedChildren(sourceIndex);

        qDebug() << sourceIndex.data(Qt::CheckStateRole).toInt() << sourceRow << sourceParent;
    }

    if (!show) {
        return false;
    } else {
        return KRecursiveFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
}

bool EwsSubscriptionFilterModel::hasCheckedChildren(QModelIndex index) const
{
    QModelIndex child;
    int row = 0;
    child = index.child(row, 0);
    while (child.isValid()) {
        if (child.data(Qt::CheckStateRole).toInt() == Qt::Checked) {
            return true;
        } else if (hasCheckedChildren(child)) {
            return true;
        }
        child = index.child(++row, 0);
    }

    return false;
}

void EwsSubscriptionFilterModel::setFilterSelected(bool enabled)
{
    mFilterSelected = enabled;
    invalidateFilter();
}

EwsSubscriptionWidgetPrivate::EwsSubscriptionWidgetPrivate(EwsClient &client, Settings *settings,
                                                           QObject *parent)
        : QObject(parent), mClient(client), mSubscribedIdsRetrieved(false), mSettings(settings)
{
}

EwsSubscriptionWidgetPrivate::~EwsSubscriptionWidgetPrivate()
{
}

void EwsSubscriptionWidgetPrivate::enableCheckBoxToggled(bool checked)
{
    mSubContainer->setEnabled(checked);
}

void EwsSubscriptionWidgetPrivate::reloadFolderList(bool)
{
    if (mClient.isConfigured()) {
        EwsFindFolderRequest *req = new EwsFindFolderRequest(mClient, this);
        EwsFolderShape shape(EwsShapeIdOnly);
        shape << EwsPropertyField("folder:DisplayName");
        shape << EwsPropertyField("folder:ParentFolderId");
        req->setFolderShape(shape);
        req->setParentFolderId(EwsId(EwsDIdMsgFolderRoot));
        connect(req, &EwsRequest::result, this, &EwsSubscriptionWidgetPrivate::readFolderListFinished);
        req->start();
        mFolderListPendingRequests = 1;
        if (!mSubscribedIdsRetrieved) {
            EwsSubscribedFoldersJob *job = new EwsSubscribedFoldersJob(mClient, mSettings, this);
            connect(job, &EwsRequest::result, this, &EwsSubscriptionWidgetPrivate::subscribedFoldersJobFinished);
            job->start();
            mFolderListPendingRequests++;
        }
        mRefreshButton->setEnabled(false);

    } else {
        mMsgWidget->setText(i18nc("@info", "Exchange server not configured."));
        mMsgWidget->setMessageType(KMessageWidget::Error);
        mMsgWidget->animatedShow();
    }
}

void EwsSubscriptionWidgetPrivate::readFolderListFinished(KJob *job)
{
    if (job->error()) {
        mMsgWidget->setText(i18nc("@info", "Failed to retrieve folder list."));
        mMsgWidget->setMessageType(KMessageWidget::Error);
        mMsgWidget->animatedShow();
        mRefreshButton->setEnabled(true);
    } else {
        EwsFindFolderRequest *req = qobject_cast<EwsFindFolderRequest*>(job);
        Q_ASSERT(req);

        mFolders = req->folders();

        mFolderListPendingRequests--;
        if (mFolderListPendingRequests == 0) {
            mRefreshButton->setEnabled(true);
            populateFolderTree();
        }
    }
}

void EwsSubscriptionWidgetPrivate::subscribedFoldersJobFinished(KJob *job)
{
    if (job->error()) {
        mMsgWidget->setText(i18nc("@info", "Failed to retrieve folder list."));
        mMsgWidget->setMessageType(KMessageWidget::Error);
        mMsgWidget->animatedShow();
        mRefreshButton->setEnabled(true);
    } else {
        EwsSubscribedFoldersJob *req = qobject_cast<EwsSubscribedFoldersJob*>(job);
        Q_ASSERT(req);

        mSubscribedIds = req->folders();

        mFolderListPendingRequests--;
        mOrigSubscribedIds = mSubscribedIds;
        mSubscribedIdsRetrieved = true;
        if (mFolderListPendingRequests == 0) {
            mRefreshButton->setEnabled(true);
            populateFolderTree();
        }
    }

}

void EwsSubscriptionWidgetPrivate::populateFolderTree()
{
    mFolderTreeModel->clear();
    mFolderItemHash.clear();

    Q_FOREACH(const EwsFolder &folder, mFolders) {
        QStandardItem *item = new QStandardItem(folder[EwsFolderFieldDisplayName].toString());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setCheckable(true);
        EwsId id = folder[EwsFolderFieldFolderId].value<EwsId>();
        item->setData(id.id(), ItemIdRole);
        if (mSubscribedIds.contains(EwsId(id.id()))) {
            item->setCheckState(Qt::Checked);
        }
        EwsId parentId = folder[EwsFolderFieldParentFolderId].value<EwsId>();
        if (parentId.type() != EwsId::Unspecified) {
            QStandardItem *parentItem = mFolderItemHash.value(parentId.id());
            if (parentItem) {
                parentItem->appendRow(item);
            }
        }
        mFolderItemHash.insert(id.id(), item);
    }

    Q_FOREACH(QStandardItem* item, mFolderItemHash) {
        if (!item->parent()) {
            mFolderTreeModel->appendRow(item);
        }
    }
}

void EwsSubscriptionWidgetPrivate::treeItemChanged(QStandardItem *item)
{
    qDebug() << "treeItemChanged";
    EwsId id = EwsId(item->data(ItemIdRole).toString());
    if (item->checkState() == Qt::Checked) {
        mSubscribedIds += id;
    } else {
        mSubscribedIds.removeOne(id);
    }
    qDebug() << mSubscribedIds;
}

void EwsSubscriptionWidgetPrivate::filterTextChanged(const QString &text)
{
    mFilterModel->setFilterFixedString(text);
}

void EwsSubscriptionWidgetPrivate::resetSelection(bool)
{
    mSubscribedIds = mOrigSubscribedIds;
    populateFolderTree();
}

EwsSubscriptionWidget::EwsSubscriptionWidget(EwsClient &client, Settings *settings, QWidget *parent)
    : QWidget(parent), d_ptr(new EwsSubscriptionWidgetPrivate(client, settings, this))
{
    Q_D(EwsSubscriptionWidget);

    d->mEnabled = d->mSettings->serverSubscription();

    QVBoxLayout *topLayout = new QVBoxLayout(this);

    d->mMsgWidget = new KMessageWidget(this);
    d->mMsgWidget->setVisible(false);

    d->mEnableCheckBox = new QCheckBox(i18nc("@option:check", "Enable server-side subscriptions"), this);
    d->mEnableCheckBox->setChecked(d->mEnabled);

    d->mSubContainer = new QWidget(this);
    QVBoxLayout *subContainerLayout = new QVBoxLayout(d->mSubContainer);
    subContainerLayout->setMargin(0);

    QLineEdit *filterLineEdit = new QLineEdit(this);
    filterLineEdit->setPlaceholderText(i18nc("@label:textbox", "Filter folders"));

    QWidget *treeContainer = new QWidget(this);
    QHBoxLayout *treeContainerLayout = new QHBoxLayout(treeContainer);
    treeContainerLayout->setMargin(0);

    d->mFolderTreeView = new QTreeView(this);
    d->mFolderTreeModel = new QStandardItemModel(this);
    d->mFilterModel = new EwsSubscriptionFilterModel(this);
    d->mFilterModel->setSourceModel(d->mFolderTreeModel);
    d->mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->mFilterModel->sort(0, Qt::AscendingOrder);
    d->mFolderTreeView->setModel(d->mFilterModel);
    d->mFolderTreeView->header()->hide();

    QWidget *buttonContainer = new QWidget(this);
    QVBoxLayout *buttonContainerLayout = new QVBoxLayout(buttonContainer);
    buttonContainerLayout->setMargin(0);

    d->mRefreshButton = new QPushButton(this);
    d->mRefreshButton->setText(i18nc("@action:button", "Reload &List"));

    QPushButton *resetButton = new QPushButton(this);
    resetButton->setText(i18nc("@action:button", "&Reset"));

    buttonContainerLayout->addWidget(d->mRefreshButton);
    buttonContainerLayout->addWidget(resetButton);
    buttonContainerLayout->addStretch();

    treeContainerLayout->addWidget(d->mFolderTreeView);
    treeContainerLayout->addWidget(buttonContainer);

    QCheckBox *subOnlyCheckBox = new QCheckBox(i18nc("@option:check", "Subscribed only"), this);

    subContainerLayout->addWidget(filterLineEdit);
    subContainerLayout->addWidget(treeContainer);
    subContainerLayout->addWidget(subOnlyCheckBox);

    topLayout->addWidget(d->mMsgWidget);
    topLayout->addWidget(d->mEnableCheckBox);
    topLayout->addWidget(d->mSubContainer);

    connect(d->mEnableCheckBox, &QCheckBox::toggled, d, &EwsSubscriptionWidgetPrivate::enableCheckBoxToggled);
    connect(d->mRefreshButton, &QPushButton::clicked, d, &EwsSubscriptionWidgetPrivate::reloadFolderList);
    connect(resetButton, &QPushButton::clicked, d, &EwsSubscriptionWidgetPrivate::resetSelection);
    connect(d->mFolderTreeModel, &QStandardItemModel::itemChanged, d, &EwsSubscriptionWidgetPrivate::treeItemChanged);
    connect(filterLineEdit, &QLineEdit::textChanged, d, &EwsSubscriptionWidgetPrivate::filterTextChanged);
    connect(subOnlyCheckBox, &QCheckBox::toggled, d->mFilterModel, &EwsSubscriptionFilterModel::setFilterSelected);

    d->enableCheckBoxToggled(d->mEnabled);
    d->reloadFolderList(false);
}

EwsSubscriptionWidget::~EwsSubscriptionWidget()
{
}

QStringList EwsSubscriptionWidget::subscribedList() const
{
    Q_D(const EwsSubscriptionWidget);

    QStringList list;
    Q_FOREACH(const EwsId &id, d->mSubscribedIds) {
        list.append(id.id());
    }

    return list;
}

bool EwsSubscriptionWidget::subscriptionEnabled() const
{
    Q_D(const EwsSubscriptionWidget);

    return d->mEnableCheckBox->isChecked();
}

bool EwsSubscriptionWidget::subscribedListValid() const
{
    Q_D(const EwsSubscriptionWidget);

    return d->mSubscribedIdsRetrieved;
}

#include "ewssubscriptionwidget.moc"
