#include <QtCore/QCoreApplication>

#include "ewsclient/ewsclient.h"

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);


    EwsClient cli(argv[1]);

    cli.getServiceConfiguration(QVector<EwsClient::RequestedConfiguration>() << EwsClient::UnifiedMessagingConfiguration);

    app.exec();

    return 0;
}
