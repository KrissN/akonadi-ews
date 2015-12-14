#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "ewsclient/ewsclient.h"
#include "ewsclient/ewsgetfolderrequest.h"

class Test : public QObject
{
    Q_OBJECT
public:
    Test(QString url, QObject *parent);

public Q_SLOTS:
    void requestFinished();

private:
    EwsClient cli;
    EwsGetFolderRequest *folderReq;
};

Test::Test(QString url, QObject *parent)
    : QObject(parent), cli(url)
{
    folderReq = new EwsGetFolderRequest(&cli);
    folderReq->setFolderId(EwsFolderId(EwsDIdInbox));
    folderReq->setFolderShape(EwsShapeDefault);
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
        EwsFolderBase *folder = folderReq->folder();
        if (!folder) {
            qDebug() << QStringLiteral("Null folder");
        }
        else {
            EwsFolderId id = folder->id();
            qDebug() << id.id() << id.changeKey();
        }
    }
}

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);

    Test test(QString::fromUtf8(argv[1]), &app);

    app.exec();

    return 0;
}

#include "ewstest.moc"
