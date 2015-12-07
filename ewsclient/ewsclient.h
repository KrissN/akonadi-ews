#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <KIO/TransferJob>
#include <QtCore/QTimer>

class EwsClient : public QObject
{
    Q_OBJECT
public:
    EwsClient(QString url, QObject *parent = 0);
    ~EwsClient();

    void setUsername(QString username, QString password)
    {
        mUsername = username;
        mPassword = password;
    }

    enum RequestedConfiguration 
    {
        MailTips = 0,
        UnifiedMessagingConfiguration,
        ProtectionRules
    };

    void getServiceConfiguration(QVector<RequestedConfiguration> conf);
    
    QString url() const 
    {
        return mUrl;
    }
    
private slots:

private:
    void sendRequest(QString body);
    
    QString mUrl;
    QString mUsername;
    QString mPassword;
    
    QTimer timer;
    int reqCnt;
};

