#include <QtCore/QDebug>
#include <QtCore/QString>

#include "ewsclient.h"

#include "ewsclient_debug.h"

EwsClient::EwsClient(QObject *parent)
    : QObject(parent)
{
    
}

EwsClient::~EwsClient()
{
}

void EwsClient::setServerVersion(const EwsServerVersion &version)
{
    if (mServerVersion.isValid() && mServerVersion != version) {
        qCWarning(EWSRES_LOG) << "Warning - server version changed." << mServerVersion << version;
    }
    mServerVersion = version;
}
