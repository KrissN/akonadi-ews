#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "ewsclient/ewsclient.h"
#include "ewsclient/ewsfindfolderrequest.h"

class Test : public QObject
{
    Q_OBJECT
public:
    Test(QString url, QObject *parent);

public Q_SLOTS:
    void requestFinished();

private:
    EwsClient cli;
    EwsFindFolderRequest *folderReq;
};

Test::Test(QString url, QObject *parent)
    : QObject(parent)
{
    cli.setUrl(url);
    folderReq = new EwsFindFolderRequest(&cli);
    folderReq->setParentFolderId(EwsId(EwsDIdMsgFolderRoot));
    EwsFolderShape shape(EwsShapeAllProperties);
    /*shape << EwsPropertyField("folder:DisplayName") << EwsPropertyField("folder:UnreadCount")
                    << EwsPropertyField("folder:ChildFolderCount") << EwsPropertyField(0x3613, EwsPropTypeString);*/
    folderReq->setFolderShape(shape);
    connect(folderReq, &EwsRequest::finished, this, &Test::requestFinished);
    folderReq->send();
}

void Test::requestFinished()
{
    qDebug() << QStringLiteral("Request done") << folderReq->isError();
    if (folderReq->isError()) {
        qDebug() << folderReq->errorString();
    }
    else {
        /*EwsFolderBase *folder = folderReq->folder();
        if (!folder) {
            qDebug() << QStringLiteral("Null folder");
        }
        else {
            EwsFolderId id = folder->id();
            qDebug() << id.id() << id.changeKey();
        }*/
        Q_FOREACH (const QPointer<EwsFolderBase> folder, folderReq->folders()) {
            qDebug() << folder->folderProperty(EwsPropertyField(0x3613, EwsPropTypeString)).toString();
        }
    }
}

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);

    Test test(QString::fromUtf8(argv[1]), &app);

    qDebug() << (EwsPropertyField(0x3613, EwsPropTypeString) == EwsPropertyField(0x3613, EwsPropTypeString));

    app.exec();

    return 0;
}

#include "ewstest.moc"
