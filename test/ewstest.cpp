#include <QtCore/QCoreApplication>

#include "ewsclient/ewsclient.h"
#include "ewsclient/ewsgetfolderrequest.h"

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);


    EwsClient cli(argv[1]);

    //cli.getServiceConfiguration(QVector<EwsClient::RequestedConfiguration>() << EwsClient::UnifiedMessagingConfiguration);
    EwsGetFolderRequest *getFolderReq = new EwsGetFolderRequest(&cli);
    getFolderReq->setDistinguishedFolderId(EwsDistinguishedFolderIdItem::Inbox);
    getFolderReq->setFolderShape(EwsBaseShapeItem::Default);
    getFolderReq->send();

    app.exec();

    return 0;
}
