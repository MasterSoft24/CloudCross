/*
    CloudCross: Opensource program for syncronization of local files and folders with Google Drive

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation version 2
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "include/msrequest.h"
#include <QDebug>




MSRequest::MSRequest()
{
    // init members on create object

    this->url=new QUrl();
    this->manager=new QNetworkAccessManager();
    this->query=new QUrlQuery();

    this->lastReply=0;

    this->replyError= QNetworkReply::NetworkError::NoError;

    this->loop=new QEventLoop(this);


    connect((QObject*)this->manager,SIGNAL(finished(QNetworkReply*)),(QObject*)this,SLOT(requestFinished(QNetworkReply*)));

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
    this->lastReply=reply;
    this->replyHeaders=reply->rawHeaderPairs();
    this->replyText=reply->readAll();
    this->replyError=reply->error();
    this->replyErrorText=reply->errorString();
    this->replyAttribute=reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    this->replyURL=reply->url().toString();
//    delete(reply);

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



