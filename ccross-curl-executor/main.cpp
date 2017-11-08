#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include "mshttprequest.h"
//#include "msnetworkproxy.h"
#include <QDataStream>
#include "qmultibuffer.h"

#include <iostream>
#include <string>

//#include <QFile>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//    QFile log("executor.log");
//    log.open(QIODevice::WriteOnly);

    MSNetworkCookieJar* jar;

    std::string idt;
    std::cin >> idt;

    QByteArray b64(idt.c_str());
//    QByteArray b64("AAAACABQAE8AUwBUAAAAXABoAHQAdABwAHMAOgAvAC8AYQBwAGkALgBkAHIAbwBwAGIAbwB4AGEAcABpAC4AYwBvAG0ALwAyAC8AZgBpAGwAZQBzAC8AbABpAHMAdABfAGYAbwBsAGQAZQByAAAAAAAAAAMAAAAcAEMAbwBuAHQAZQBuAHQALQBMAGUAbgBnAHQAaAAAAAYAMQAwADUAAAAaAEEAdQB0AGgAbwByAGkAegBhAHQAaQBvAG4AAACOAEIAZQBhAHIAZQByACAAMwBNADkARQBMAEMANQBwAFAAdABBAEEAQQBBAEEAQQBBAEEAQQBGAEgAYwBPAFcAcwBwAG0AYgBNAGUAaQBXAFIAYwA1AFkAdwB6AEcAMAAwAG8ARgAzAFgAOABHAHQAVgByAEwASgBqAEMAZwA4AG8AdQBjAGEAaQBnAGEAZAAAABgAQwBvAG4AdABlAG4AdAAtAFQAeQBwAGUAAAA+AGEAcABwAGwAaQBjAGEAdABpAG8AbgAvAGoAcwBvAG4AOwAgAGMAaABhAHIAcwBlAHQAPQBVAFQARgAtADgAAAAAAAAAAAAAAAAAAAAAAAAAGGh0dHA6Ly8xNTguNjkuMTk3LjIzNjo4MAAAAAlOT19DT09LSUUAAAALTk9fT1VURklMRQACAAAAaXsKICAgICJpbmNsdWRlX2RlbGV0ZWQiOiBmYWxzZSwKICAgICJpbmNsdWRlX21lZGlhX2luZm8iOiBmYWxzZSwKICAgICJwYXRoIjogIiIsCiAgICAicmVjdXJzaXZlIjogdHJ1ZQp9Cg==");

    MSNetworkProxy* prx = new MSNetworkProxy();

    MSHttpRequest* req = new MSHttpRequest(0);

    QDataStream ds(QByteArray::fromBase64(b64));

    ds >> req->requestMethod;
    ds >> req->requestURL;
    ds >> req->queryItems;
    ds >> req->requestHeaders;

    ds >> req->cUrlObject->payloadChunkSize;
    ds >> req->cUrlObject->payloadFilePosition;

    QByteArray proxy;
    ds >> proxy;

    if(QString(proxy) != "NO_PROXY"){

        prx->setProxyFromString(proxy);
        req->setProxy(prx);
    }
    else{
        req->disableProxy();
    }


    QByteArray cookie;
    ds >> cookie;
    if((QString(cookie) != "NO_COOKIE")&&(QString(cookie) != "EMPTY_COOKIE")){

        // set cookie object

        jar = new MSNetworkCookieJar(req);
        jar->cookieFile->seek(0);
        jar->cookieFile->write(cookie);
        jar->cookieFile->flush();
        jar->cookieFile->close();

        req->MSsetCookieJar(jar);
        //req->cookieJarObject->cookieFile->open(QIODevice::ReadWrite);

    }
    else{
        if(QString(cookie) == "EMPTY_COOKIE"){
            jar = new MSNetworkCookieJar(req);
            req->MSsetCookieJar(jar);
        }

    }

    QByteArray outfile;
    ds >> outfile;
    if(QString(outfile) != "NO_OUTFILE"){

        req->cUrlObject->outFile = new QFile(outfile);
    }



    MSHttpRequest::dataStreamTypes dst;
    quint8 dstt;
    ds >> dstt;
    dst = (MSHttpRequest::dataStreamTypes)dstt;

    if(dst != MSHttpRequest::dataStreamTypes::DS_None){

        if(dst == MSHttpRequest::dataStreamTypes::DS_ByteArray){

            QByteArray par;
            ds >> par;

            if( req->requestMethod.toUpper() == "POST" ){

                req->post(par);
            }
            if( req->requestMethod.toUpper() == "PUT" ){

                req->put(par);
            }

        }

        if(dst == MSHttpRequest::dataStreamTypes::DS_File){
            QByteArray fnm;
            ds >> fnm;
            QFile par(fnm);

            if( req->requestMethod.toUpper() == "POST" ){

                req->post(&par);
            }
            if( req->requestMethod.toUpper() == "PUT" ){

                req->put(&par);
            }
        }

        if(dst == MSHttpRequest::dataStreamTypes::DS_MultiBuffer){

            QMultiBuffer par;
            int sc;
            ds >> sc;

            for(int i=0; i < sc; i++){

                int dst;
                QByteArray ba;
                ds >> dst;
                ds >> ba;

                if((MSHttpRequest::dataStreamTypes)dst == MSHttpRequest::dataStreamTypes::DS_ByteArray){

                    par.append(new QByteArray(ba));
                }
                else{
                    par.append(new QFile(ba));
                }


            }

            if( req->requestMethod.toUpper() == "POST" ){

                req->post(&par);
            }
            if( req->requestMethod.toUpper() == "PUT" ){

                req->put(&par);
            }

//            for(int i=0; i < sc; i++){// free allocated resources

//                delete(qvariant_cast<QIODevice*> (par.items[i].slot));
//            }
        }
    }
    else{

            if( req->requestMethod.toUpper() == "POST" ){

                req->post("");
            }

            if( req->requestMethod.toUpper() == "DELETE" ){

                req->deleteResource();
            }

            if( req->requestMethod.toUpper() == "GET" ){

                req->get();
            }
    }

    /* The Data for sending to a main process
     * replyHeaders
     * buffer()
     * lastError()
     * errorBuffer()
     * replyURL
     * cookieFile
     */

    QByteArray d;
    QDataStream rds(&d,QIODevice::WriteOnly);

    QByteArray e;

    QByteArray b (req->cUrlObject->_buffer.c_str());

    QString bs(b);

    rds << req->cUrlObject->replyHeaders;
    rds << QByteArray(req->cUrlObject->_buffer.c_str());
    rds << (uint)(req->cUrlObject->lastError().code());
    rds << QByteArray(req->cUrlObject->errorBuffer().toStdString().c_str());

    QByteArray ru(req->cUrlObject->replyURL.toLocal8Bit());
    rds << ru;

    if((req->cookieJarObject == nullptr)||(req->cookieJarObject->cookieFile->size() == 0)){
        rds << QByteArray("NO_COOKIE");
    }
    else{

        req->cookieJarObject->cookieFile->seek(0);
        rds << req->cookieJarObject->cookieFile->readAll();
    }


    if(req->cookieJarObject != nullptr){
        delete(req->cookieJarObject);
    }


    std::cout << d.toBase64().toStdString().c_str()  ;

    std::cout << std::flush;

     //delete(req);
    //req->deleteLater();

return 0;
//    return a.exec();
}
