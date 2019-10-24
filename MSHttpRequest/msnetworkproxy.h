#ifndef MSNETWORKPROXY_H
#define MSNETWORKPROXY_H

#include <QObject>


class MSNetworkProxy : public QObject
{

   Q_OBJECT


public:
    enum ProxyType {
        DefaultProxy,
        Socks5Proxy,
        NoProxy,
        HttpProxy,
        HttpCachingProxy,
        FtpCachingProxy
    };



    QString proxyType;
    QString proxyAddr;
    QString proxyPort;


    MSNetworkProxy(){

    }

    ~MSNetworkProxy(){

    }

    QString getProxyString(){

        if(this->proxyAddr != ""){
            return this->proxyType + "://" + this->proxyAddr + ":" + this->proxyPort;
        }

        return QString("ggggggg");

    }

    void setProxyFromString(QString addr){ // could be a schema://00.00.00.00:00 or a 00.00.00.00:00

        QString schema = "";
        QString host;
        QString port;

        // test for schema
        if(addr.contains("http") || addr.contains("socks5")){
            schema = addr.left(addr.indexOf(":"));
            addr = addr.mid(addr.indexOf(":")+3);
        }
        else{
            schema = "http";
        }

        // extract address

        host = addr.left(addr.lastIndexOf(":"));

        //extract port

        port = addr.mid(addr.lastIndexOf(":")+1);

        // set proxy data

        this->proxyAddr = host;
        this->proxyPort = port;
        this->proxyType = schema;
    }

    void setHostName(QString host){

        this->proxyAddr = host;
    }

    void setPort(qint32 port){

        this->proxyPort = QString::number(port);

    }

    void setType(MSNetworkProxy::ProxyType type){

        switch (type) {
        case MSNetworkProxy::HttpProxy:

            this->proxyType = "http";
            break;

        case MSNetworkProxy::Socks5Proxy:

            this->proxyType = "socks5";
            break;
        default:
            this->proxyType = "http";
            break;
        }

    }
};


#endif // MSNETWORKPROXY_H
