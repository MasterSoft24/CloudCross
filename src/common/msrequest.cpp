#include "include/msrequest.h"
#include <QDebug>
#include <QEventLoop>



MSRequest::MSRequest()
{
    // init members on create object

    this->url=new QUrl();
    this->manager=new QNetworkAccessManager();
    this->query=new QUrlQuery();


    connect((QObject*)this->manager,SIGNAL(finished(QNetworkReply*)),(QObject*)this,SLOT(requestFinished(QNetworkReply*)));

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

    this->setRawHeader(QByteArray::fromStdString(headerName.toStdString())  ,QByteArray::fromStdString(headerValue.toStdString()));

}


void MSRequest::addHeader(QByteArray headerName, QByteArray headerValue){

    this->setRawHeader(headerName,headerValue);

}


void MSRequest::methodCharger(QNetworkRequest req){

    QNetworkReply* replySync;

    if(this->requestMethod=="get"){
        replySync=this->manager->get(req);
    }


    if(this->requestMethod=="post"){
        req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        QByteArray ba;
        ba+=this->query->toString();
        replySync=this->manager->post(req,ba);
    }

    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();

}


void MSRequest::post(QByteArray data){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    //this->setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    replySync=this->manager->post(*this,data);

    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();
}



void MSRequest::put(QByteArray data){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    //this->setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    replySync=this->manager->put(*this,data);

    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();
}


void MSRequest::exec(){

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    methodCharger(*this);

    // 301 redirect handling
    while(true){

        QVariant possible_redirected_url= this->lastReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if(!possible_redirected_url.isNull()) {

            QUrl rdrUrl=possible_redirected_url.value<QUrl>();

            QString reqStr=rdrUrl.scheme()+"://"+rdrUrl.host()+"/"+rdrUrl.path()+"?"+rdrUrl.query();

            methodCharger(QNetworkRequest(reqStr));

        }
        else{
            break;
        }
    }

}


void MSRequest::requestFinished(QNetworkReply *reply){

    this->lastReply= reply;

}










