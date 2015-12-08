#include <QtCore/QDebug>
#include <QtCore/QString>

#include "ewsclient.h"

#include "ewsrequestwriter.h"


EwsClient::EwsClient(QString url, QObject *parent)
    : QObject(parent), mUrl(url)
{
    
}

EwsClient::~EwsClient()
{
}

void EwsClient::getServiceConfiguration(QVector<RequestedConfiguration> conf)
{
    EwsRequestWriter reqWriter;

    reqWriter.writeEwsMsgStartElement(QStringLiteral("GetServiceConfiguration"));
    reqWriter.writeEwsMsgStartElement(QStringLiteral("RequestedConfiguration"));

    foreach (RequestedConfiguration c, conf)
    {
        QString confStr;
        switch (c)
        {
            case MailTips:
                confStr = QStringLiteral("MailTips");
                break;
            case UnifiedMessagingConfiguration:
                confStr = QStringLiteral("UnifiedMessagingConfiguration");
                break;
            case ProtectionRules:
                confStr = QStringLiteral("ProtectionRules");
                break;
            default:
                qWarning() << "Invalid service configuration option";
                return;
        }
        reqWriter.writeEwsMsgStartElement(QStringLiteral("ConfigurationName"));
        reqWriter.writeCharacters(confStr);
        reqWriter.writeEndElement();
    }
    reqWriter.writeEndElement();
    reqWriter.writeEndElement();

    qDebug() << reqWriter.toString();

    sendRequest(reqWriter.toString());
}
        
void EwsClient::sendRequest(QString body)
{
#if 0
/*    QNetworkReply* reply = naMgr.get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, [this, reply]{requestFinished(reply);});
    QObject::connect(reply, &QNetworkReply::readyRead, [this, reply]{requestReadyRead(reply);});*/
    //KIO::MetaData md;
    KIO::TransferJob *job = KIO::http_post(mUrl, body.toUtf8(), KIO::HideProgressInfo);
    job->addMetaData("content-type", "text/xml");
    /*QObject::connect(job, static_cast<void (KIO::MultiGetJob::*)(long, const QByteArray&)>(&KIO::MultiGetJob::data),
                     [this, job](long id, const QByteArray &data){requestDataReady(job, id, data);});
    QObject::connect(job, static_cast<void (KIO::MultiGetJob::*)(long)>(&KIO::MultiGetJob::result),
                     [this, job](long id){qDebug() << "Result" << job->errorString();});
                     QObject::connect(job, &KIO::MultiGetJob::dataReq, [this](KIO::Job *job, QByteArray &data){qDebug() << "dataReq";});*/
    QObject::connect(job, &KIO::TransferJob::result, [this](KJob *job){
            qDebug() << "result" << job->error();
            reqCnt++;
            /*if (reqCnt < 3)
              getServiceConfiguration({EwsClient::MailTips});*/
            job->deleteLater();
        });
    job->start();
    

    QObject::connect(&timer, &QTimer::timeout, [job]{job->deleteLater();});
    timer.start(30 * 1000);
#endif       
}

