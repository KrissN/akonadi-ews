#include <QtCore/QString>
#include <QtCore/QXmlStreamWriter>

class EwsRequestWriter : public QXmlStreamWriter
{
public:
    static const QString soapEnvNsUri;
    static const QString ewsMsgNsUri;
    static const QString ewsTypeNsUri;
    
    EwsRequestWriter();
    ~EwsRequestWriter();
    
    QString toString();

    void writeEwsTypeStartElement(const QString & name)
    {
        writeStartElement(ewsTypeNsUri, name);
    }
    
    void writeEwsMsgStartElement(const QString & name)
    {
        writeStartElement(ewsMsgNsUri, name);
    }
    
private:
    QString req;
    bool finished;
};
