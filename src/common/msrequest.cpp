/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.
    All rights reserved.


  BSD License

  Redistribution and use in source and binary forms, with or without modification, are
  permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice, this list
    of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.
  - Neither the name of the "Vladimir Kamensky" or "Master Soft LLC." nor the names of
    its contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY E
  XPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES O
  F MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SH
  ALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENT
  AL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROC
  UREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS I
  NTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRI
  CT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF T
  HE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "include/msrequest.h"
#include <QDebug>




MSRequest::MSRequest(QNetworkProxy *proxy)
{
    // init members on create object

    this->url=new QUrl();
    this->manager=new QNetworkAccessManager();
    this->query=new QUrlQuery();

    //this->cookieJar=0;

    this->lastReply=0;
    this->outFile=0;

    this->replyError= QNetworkReply::NetworkError::NoError;

    this->loop=new QEventLoop(this);


    connect((QObject*)this->manager,SIGNAL(finished(QNetworkReply*)),(QObject*)this,SLOT(requestFinished(QNetworkReply*)));

    if(proxy != 0){
        this->setProxy(proxy);
    }
}


MSRequest::~MSRequest(){

    delete(this->loop);
    delete(this->manager);

    //QNetworkRequest::~QNetworkRequest();
}


void MSRequest::setRequestUrl(QString url){

    this->url->setUrl( url);

}


void MSRequest::addQueryItem(QString itemName, QString itemValue){

        this->query->addQueryItem(itemName,itemValue);

}


bool MSRequest::setMethod(QString method){
    if((method=="post")||(method=="get")|(method=="put")){
        this->requestMethod=method;
        return true;
    }
    else{
        return false;
    }
}


void MSRequest::addHeader(QString headerName, QString headerValue){

    //this->setRawHeader(QByteArray::fromStdString(headerName.toStdString())  ,QByteArray::fromStdString(headerValue.toStdString()));
    this->setRawHeader(QByteArray(headerName.toStdString().c_str()),QByteArray(headerValue.toStdString().c_str()));

}


void MSRequest::addHeader(QByteArray headerName, QByteArray headerValue){

    this->setRawHeader(headerName,headerValue);

}


void MSRequest::methodCharger(QNetworkRequest req){

    QNetworkReply* replySync=0;

    if(this->requestMethod=="get"){
        replySync=this->manager->get(req);
    }


    if(this->requestMethod=="post"){
        req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        QByteArray ba;
        ba+=this->query->toString();
        replySync=this->manager->post(req,ba);
    }

//    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();
}


void MSRequest::methodCharger(QNetworkRequest req,QString path){

    // fix warning message

    path=path+"";

    QNetworkReply* replySync=0;

    if(this->requestMethod=="get"){
        replySync=this->manager->get(req);
    }


    if(this->requestMethod=="post"){
        req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        QByteArray ba;
        ba+=this->query->toString();
        replySync=this->manager->post(req,ba);
    }

    this->currentReply=replySync;

    connect(replySync,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(doDownloadProgress(qint64,qint64)));
    connect(replySync,SIGNAL(readyRead()),this,SLOT(doReadyRead()));

//    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();
}



void MSRequest::post(QByteArray data){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    replySync=this->manager->post(*this,data);

    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //this->loop.deleteLater();
}


void MSRequest::download(QString url){

    //this->setUrl(*this->url);
    this->setUrl(QUrl(url));


    methodCharger(*this);

    // 301 redirect handling
    while(true){

        QVariant possible_redirected_url= this->replyAttribute;//this->lastReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if(!possible_redirected_url.isNull()) {

            QUrl rdrUrl=possible_redirected_url.value<QUrl>();

           // QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+"/"+rdrUrl.path()+"?"+rdrUrl.query();
            QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+""+rdrUrl.path()+"?"+rdrUrl.query();

            methodCharger(QNetworkRequest(reqStr));

        }
        else{
            break;
        }
    }
}



void MSRequest::download(QString url,QString path){

    this->setUrl(QUrl(url));

    this->outFile= new QFile(path);
    this->outFile->open(QIODevice::WriteOnly );




    methodCharger(*this,path);

    // 301 redirect handling
    while(true){

        QVariant possible_redirected_url= this->replyAttribute;//this->lastReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if(!possible_redirected_url.isNull()) {

            QUrl rdrUrl=possible_redirected_url.value<QUrl>();

           // QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+"/"+rdrUrl.path()+"?"+rdrUrl.query();
            QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+""+rdrUrl.path()+"?"+rdrUrl.query();

            methodCharger(QNetworkRequest(reqStr),path);

        }
        else{
            break;
        }
    }

}


void MSRequest::put(QByteArray data){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    replySync=this->manager->put(*this,data);

    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();
}


void MSRequest::put(QIODevice *data){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    replySync=this->manager->put(*this,data);

    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();
}



void MSRequest::deleteResource(){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    replySync=this->manager->deleteResource(*this);

    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();
}




void MSRequest::exec(){

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    methodCharger(*this);

    // 301 redirect handling
    while(true){

        QVariant possible_redirected_url= this->replyAttribute;//this->lastReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if(!possible_redirected_url.isNull()) {

            QUrl rdrUrl=possible_redirected_url.value<QUrl>();

           // QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+"/"+rdrUrl.path()+"?"+rdrUrl.query();
            QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+""+rdrUrl.path()+"?"+rdrUrl.query();

            methodCharger(QNetworkRequest(reqStr));

        }
        else{
            break;
        }
    }

}


void MSRequest::requestFinished(QNetworkReply *reply){

    if(this->outFile!=0){

        this->outFile->write(this->currentReply->readAll());
        this->outFile->close();
        delete(this->outFile);
        this->outFile=0;
    }

    this->lastReply=reply;
    this->replyHeaders=reply->rawHeaderPairs();
    this->replyText=reply->readAll();
    this->replyError=reply->error();
    this->replyErrorText=reply->errorString();
    this->replyAttribute=reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    this->replyURL=reply->url().toString();
//    delete(reply);

}



void MSRequest::doDownloadProgress(qint64 avail, qint64 total){

   // fix warning message
    if(avail==total){
        return;
    }

}


void MSRequest::doReadyRead(){

    this->outFile->write(this->currentReply->readAll());
}


QByteArray MSRequest::readReplyText(){
    return this->replyText;
}


bool MSRequest::replyOK(){

    if(this->replyError == QNetworkReply::NetworkError::NoError){
        return true;
    }
    else{
        return false;
    }
}


void MSRequest::printReplyError(){

    qStdOut() << this->replyErrorText << endl;
}



void MSRequest::MSsetCookieJar(QNetworkCookieJar* cookie)
{

//    if(*cookie == 0){
//        *cookie=new QNetworkCookieJar();
//        this->cookieJar=*cookie;
//    }
//    else{
//        this->cookieJar=*cookie;
//    }


    this->cookieJar=cookie;
    this->manager->setCookieJar(this->cookieJar);

}



QJsonObject MSRequest::cookieToJSON()
{
    QJsonObject c;
    //QList<QNetworkCookie> cl=this->manager->cookieJar()->cookiesForUrl(*this->url);
    QList<QNetworkCookie> cl=this->manager->cookieJar()->cookiesForUrl( QUrl(this->replyURL));


    for(int i=0;i < cl.size();i++){

        c.insert(cl[i].name(),QJsonValue::fromVariant(QVariant(cl[i].value())));
    }

    return c;
}



bool MSRequest::cookieFromJSON(QJsonObject cookie)
{

}


void MSRequest::setProxy(QNetworkProxy *proxy){

//QNetworkProxy* proxy = new QNetworkProxy(QNetworkProxy::HttpProxy, "209.203.144.69", 8080);

//QNetworkProxy* proxy = new QNetworkProxy();
//proxy->setType(QNetworkProxy::HttpProxy);
//proxy->setHostName("154.16.127.214");
//proxy->setPort(80);

this->manager->setProxy(*proxy);

}



