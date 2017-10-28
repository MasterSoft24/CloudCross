#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include "mshttprequest.h"
#include <QDataStream>
#include "qmultibuffer.h"

#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MSNetworkCookieJar* jar;

    std::string idt;
    std::cin >> idt;

    QByteArray b64(idt.c_str());
//    QByteArray b64("AAAACABQAE8AUwBUAAAASgBoAHQAdABwAHMAOgAvAC8AYwBsAG8AdQBkAC4AbQBhAGkAbAAuAHIAdQAvAGEAcABpAC8AdgAyAC8AZgBpAGwAZQAvAGEAZABkAAAACgAAAA4AeAAtAGUAbQBhAGkAbAAAACYAbQBlAGcAYQBsAGkAbwBuADcAMwBAAGkAbgBiAG8AeAAuAHIAdQAAAAgAcwBpAHoAZQAAAAoAMQAyADAAMgAwAAAACgB0AG8AawBlAG4AAABAADgAdgBRAGIAYwB3ADEATQBUAGkATAA3AHgANABZAHkANQBiADgAVgB3AFYAUAA4AHUAUwBUAGkAbwB2AEQANAAAAAgAaABhAHMAaAAAAFAARAAxADIANwA3ADgAMAA2ADQAMwA5ADgARABFADAARgA0AEUAMQA1AEUAMwAzAEYARgA5ADEAMAA5ADMAOABDADkARABBADUAOQAyAEMAQwAAAAoAZQBtAGEAaQBsAAAAJgBtAGUAZwBhAGwAaQBvAG4ANwAzAEAAaQBuAGIAbwB4AC4AcgB1AAAABgBhAHAAaQAAAAIAMgAAABAAYwBvAG4AZgBsAGkAYwB0AAAADABzAHQAcgBpAGMAdAAAAAgAaABvAG0AZQAAAC4ALwAxADAAMAAwAC0AZgBpAGwAZQBzAC8AZgBpAGwAZQAwADAAMgAuAGIAaQBuAAAAEgB4AC0AcABhAGcAZQAtAGkAZAAAABQAQgBBAEwAaABwAGsAVwBJAEsAQQAAAAoAYgB1AGkAbABkAAAAUABoAG8AdABmAGkAeABfAEMATABPAFUARABXAEUAQgAtADcANwAxADAAXwA1ADAALQAwAC0AMQAuADIAMAAxADcAMQAwADAAMgAxADcANQA4AAAAAAAAAf8jIE5ldHNjYXBlIEhUVFAgQ29va2llIEZpbGUKIyBodHRwczovL2N1cmwuaGF4eC5zZS9kb2NzL2h0dHAtY29va2llcy5odG1sCiMgVGhpcyBmaWxlIHdhcyBnZW5lcmF0ZWQgYnkgbGliY3VybCEgRWRpdCBhdCB5b3VyIG93biByaXNrLgoKI0h0dHBPbmx5Xy5hdXRoLm1haWwucnUJVFJVRQkvCVRSVUUJMTU0MDUyNTU5NAlHYXJhZ2VJRAllMmM2MzViOTEyOGQ0ZjE3OWUzOTczNDk4MjZmNTU5YgoubWFpbC5ydQlUUlVFCS8JRkFMU0UJMAlNcG9wCTE1MDg5ODk1OTQ6NjI3NTA0MGY3ZTdhNGM2MDE5MDUwMjE5MDgwNTAwMWIwNDA3MWQwNDA2NDU2ODUxNWM0NTVmMDcwMTAwMWUwYjczMDcxZTU0NWM1ZjUwNWM1ODU4NWIwMDA2MTA1ZDU5NWI1ZTQ4MTg0YTQ3Om1lZ2FsaW9uNzNAaW5ib3gucnU6Ci5tYWlsLnJ1CVRSVUUJLwlGQUxTRQkxNTI0NTQxNTk0CXQJb2JMRDFBQUFBQUFJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQkFBQUFBQUFBQUFRQUFBQ0EKAAAAC05PX09VVEZJTEUAAA==");

    MSHttpRequest* req = new MSHttpRequest();

    QDataStream ds(QByteArray::fromBase64(b64));

    ds >> req->requestMethod;
    ds >> req->requestURL;
    ds >> req->queryItems;
    ds >> req->requestHeaders;

    QByteArray cookie;
    ds >> cookie;
    if((QString(cookie) != "NO_COOKIE")&&(QString(cookie) != "")){

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
        jar = new MSNetworkCookieJar(req);
        req->MSsetCookieJar(jar);
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
     * cookieFile
     */

    QByteArray d;
    QDataStream rds(&d,QIODevice::WriteOnly);

    QByteArray e;

    QByteArray b (req->cUrlObject->_buffer.c_str());

    QString bs(b);
    //int start =  bs.indexOf("\"csrf\"");


    rds << req->cUrlObject->replyHeaders;
    rds << QByteArray(req->cUrlObject->_buffer.c_str());
    rds << (uint)(req->cUrlObject->lastError().code());
    rds << QByteArray(req->cUrlObject->errorBuffer().toStdString().c_str());

    if(req->cookieJarObject == nullptr){
        rds << QByteArray("NO_COOKIE");
    }
    else{

        req->cookieJarObject->cookieFile->seek(0);
        rds << req->cookieJarObject->cookieFile->readAll();
    }


    delete(req->cookieJarObject);

    delete(req);



    std::cout << d.toBase64().toStdString().c_str()  ;

    std::cout << std::flush;

return 0;
//    return a.exec();
}
