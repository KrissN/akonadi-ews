#include "ewsrequestwriter.h"

const QString EwsRequestWriter::soapEnvNsUri = QStringLiteral("http://schemas.xmlsoap.org/soap/envelope/");
const QString EwsRequestWriter::ewsMsgNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/messages");
const QString EwsRequestWriter::ewsTypeNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/types");

static const QString ewsReqVersion = QStringLiteral("Exchange2010");

EwsRequestWriter::EwsRequestWriter()
    : QXmlStreamWriter(&req), finished(false)
{
    setCodec("UTF-8");
    
    writeStartDocument();

    writeNamespace(soapEnvNsUri, QStringLiteral("soap"));
    writeNamespace(ewsMsgNsUri, QStringLiteral("m"));
    writeNamespace(ewsTypeNsUri, QStringLiteral("t"));

    // SOAP Envelope
    writeStartElement(soapEnvNsUri, QStringLiteral("Envelope"));

    // SOAP Header
    writeStartElement(soapEnvNsUri, QStringLiteral("Header"));
    writeStartElement(ewsTypeNsUri, QStringLiteral("RequestServerVersion"));
    writeAttribute(QStringLiteral("Version"), ewsReqVersion);
    writeEndElement();
    writeEndElement();

    // SOAP Body
    writeStartElement(soapEnvNsUri, QStringLiteral("Body"));
}

EwsRequestWriter::~EwsRequestWriter()
{
}

QString EwsRequestWriter::toString()
{
    if (!finished)
    {
        // End SOAP Body
        writeEndElement();
        
        // End SOAP Envelope
        writeEndElement();
        
        writeEndDocument();
        
        finished = true;
    }

    return req;
}

