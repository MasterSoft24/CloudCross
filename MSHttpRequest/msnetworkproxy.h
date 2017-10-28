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

        return this->proxyType + "://" + this->proxyAddr + ":" + this->proxyPort;
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
