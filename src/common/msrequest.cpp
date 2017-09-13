/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2016-2017  Vladimir Kamensky
    Copyright (C) 2016-2017  Master Soft LLC.
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

#ifndef CCROSS_LIB

    this->setQueryForDownload =true;

    this->url=new QUrl();
    this->manager=new QNetworkAccessManager();
    this->query=new QUrlQuery();

    //this->cookieJar=0;

    this->lastReply=0;
    this->outFile=nullptr;

    this->requesProcessed = false;

    this->replyError= QNetworkReply::NetworkError::NoError;

    this->loop=new QEventLoop(this);


    connect((QObject*)this->manager,SIGNAL(finished(QNetworkReply*)),(QObject*)this,SLOT(requestFinished(QNetworkReply*)));

    if(proxy != 0){
        this->setProxy(proxy);
    }
#else

    Q_UNUSED(proxy)

    cUrlObject.requestOptions[CURLOPT_COOKIEFILE].setValue(QString());
    cookieJarObject = nullptr;

#endif
}


MSRequest::~MSRequest(){

#ifndef CCROSS_LIB
    delete(this->loop);
    delete(this->manager);

#else
   // delete(this->cookieJarObject);
#endif
    //QNetworkRequest::~QNetworkRequest();
}


void MSRequest::setRequestUrl(const QString &url){
#ifdef CCROSS_LIB

    this->requestURL = QUrl::fromPercentEncoding(url.toLocal8Bit());

#endif
#ifndef CCROSS_LIB
    this->url->setUrl( url);
#endif
}


void MSRequest::addQueryItem(const QString &itemName, const QString &itemValue){
#ifdef CCROSS_LIB
        this->queryItems.insert(itemName,itemValue);
#endif
#ifndef CCROSS_LIB
        this->query->addQueryItem(itemName,itemValue);
#endif
}


bool MSRequest::setMethod(const QString &method){

    if((method=="post")||(method=="get")||(method=="put")){
        this->requestMethod=method;
        return true;
    }
    else{
        return false;
    }

}


void MSRequest::addHeader(const QString &headerName, const QString &headerValue){
#ifdef CCROSS_LIB

    this->requestHeaders.insert(headerName,headerValue);

#endif
#ifndef CCROSS_LIB
    //this->setRawHeader(QByteArray::fromStdString(headerName.toStdString())  ,QByteArray::fromStdString(headerValue.toStdString()));
    this->setRawHeader(QByteArray(headerName.toStdString().c_str()),QByteArray(headerValue.toStdString().c_str()));
#endif
}


void MSRequest::addHeader(const QByteArray &headerName, const QByteArray &headerValue){
#ifdef CCROSS_LIB

    this->requestHeaders.insert(headerName,headerValue);

#endif
#ifndef CCROSS_LIB
    this->setRawHeader(headerName,headerValue);
#endif
}



QString MSRequest::getReplyHeader(const QByteArray &headerName){

    for(int i = 0; i < this->replyHeaders.size(); i++){

        QPair<QByteArray,QByteArray> h = this->replyHeaders[i];

        if(QString(h.first) == QString(headerName)){

            return QString(h.second);
        }

    }

    return "";

}


void MSRequest::methodCharger(QNetworkRequest req){

#ifdef CCROSS_LIB

    Q_UNUSED(req);


    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->setCURLOptions();

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - CHARDGER Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }


    return;


#endif


#ifndef CCROSS_LIB
    QNetworkReply* replySync=0;

    if(this->requestMethod=="get"){

        replySync=this->manager->get(req);
    }


    if(this->requestMethod=="post"){

        if(!req.hasRawHeader("Content-Type")){

            if(!this->notUseContentType){
                req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
            }
        }



        QByteArray ba;
        ba+=this->query->toString();
        replySync=this->manager->post(req,ba);
    }

#ifdef PRINT_DEBUG_INFO
    this->printDebugInfo_request(req);
#endif

//    connect(replySync,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(doDownloadProgress(qint64,qint64)));
    //connect(replySync,SIGNAL(readyRead()),this,SLOT(doReadyRead()));


//    QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();

#endif
}


void MSRequest::printDebugInfo_request(const QNetworkRequest &req){

    qDebug()<<"";
    qDebug()<<"=========== Begin Query Debug Info Block =============";

    qDebug()<<"Request URL: "<< this->url->toString();
    qDebug()<<"Request Method: "<< this->requestMethod;
    qDebug()<<"Request Headers: ";

    QList<QByteArray> hl=req.rawHeaderList();

    for(int i=0; i< hl.size();i++){
        qDebug()<<hl.at(i)<<": "<<req.rawHeader(hl.at(i));

    }



    qDebug()<<"";
    qDebug()<<"Query parameters: ";
    QList<QPair<QString, QString> >  qi=this->query->queryItems();

    for(int i = 0;i<qi.size();i++){

        qDebug()<<qi.at(i).first<<": "<<qi.at(i).second;
    }

    qDebug()<<"=========== End Query Debug Info Block =============";
    qDebug()<<"";
}



void MSRequest::printDebugInfo_response(QNetworkReply *reply){

    qDebug()<<"";
    qDebug()<<"=========== Begin Response Debug Info Block =============";


    qDebug()<<"Response Headers: ";

    QList<QByteArray> hl=reply->rawHeaderList();

    for(int i=0; i< hl.size();i++){
        qDebug()<<hl.at(i)<<": "<<reply->rawHeader(hl.at(i));

    }

    qDebug()<<"";

    qDebug()<<"Response Body: ";
    qDebug()<<this->replyText;

    qDebug()<<"=========== End Response Debug Info Block =============";
    qDebug()<<"";
}


#ifdef CCROSS_LIB

QString MSRequest::toUrlEncoded(const QString &p){

    return QUrl::toPercentEncoding(p ,QByteArray("/,,/,\\,?,:,@,&,=,+,$,#,-,_,.,!,~,*,',(,)")) ;;// qs ;
}



void MSRequest::setCURLOptions(QIODevice* payloadPtr){ // this method for put from file only


    if(this->requestMethod != "put"){

        return;


    }
    else{

        cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL).toLocal8Bit());
        cUrlObject.requestOptions[CURLOPT_UPLOAD] = 1;
        cUrlObject.requestOptions[CURLOPT_PUT] = 1;
        cUrlObject.requestOptions[CURLOPT_INFILESIZE_LARGE] = payloadPtr->size();

        // set request headers

        if(this->requestHeaders.size() > 0){

            QStringList h;
            QHash<QString,QString>::iterator i = this->requestHeaders.begin();

            for(;i != this->requestHeaders.end();i++){

                h << QString(i.key() + ": "+i.value()).toLocal8Bit();

            }

            cUrlObject.requestOptions[CURLOPT_HTTPHEADER] = h;
        }


        return;

    }



}




void MSRequest::setCURLOptions(const QByteArray &payload){

    this->cUrlObject.requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    if(this->requestMethod != "get"){

        if((this->requestMethod == "put") || (this->requestMethod == "delete")){
            cUrlObject.requestOptions[CURLOPT_CUSTOMREQUEST] = this->requestMethod.toUpper();
            cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL).toLocal8Bit());
        }

        if(this->requestMethod == "post"){
            //cUrlObject.requestOptions[CURLOPT_POST] = 1;
            cUrlObject.requestOptions[CURLOPT_CUSTOMREQUEST] ="POST";
            cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL).toLocal8Bit());
        }

    }
    else{ // GET

        QString p = "";

        if(this->queryItems.size() > 0){

            QHash<QString,QString>::iterator i = this->queryItems.begin();

            for(;i != this->queryItems.end();i++){

                if( p != ""){
                    p += "&";
                }
                p += i.key()+"="+ cUrlObject.escape(i.value());
            }

            // hack for escape slashes in request string

            if(p.indexOf("/") != -1){

               // p=p.replace("/",cUrlObject.escape("/"));
            }

            cUrlObject.requestOptions[CURLOPT_URL] = this->requestURL.toLocal8Bit() + "?" + p.toLocal8Bit();//toUrlEncoded(this->requestURL.toLocal8Bit())+"?" + toUrlEncoded(p.toLocal8Bit());

        }
        else{
            cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL.toLocal8Bit()));//.toLocal8Bit();
            //qDebug()<< this->requestURL;
        }



    }




    if((this->queryItems.size() > 0) && (this->requestMethod != "get")){

        if(payload.size() == 0){
            QString p = "";

            QHash<QString,QString>::iterator i = this->queryItems.begin();

            for(;i != this->queryItems.end();i++){

                if( p != ""){
                    p += "&";
                }
                p += i.key()+"="+ cUrlObject.escape(i.value());
            }

            if(this->requestMethod.toUpper() == "POST"){
//                qDebug() << p;

                cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL.toLocal8Bit()));
                cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = ((p.toLocal8Bit()));
                cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = ((p.toLocal8Bit())).size() ;
            }
            else{

                cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL.toLocal8Bit())+"?"+p.toLocal8Bit());

            }

            this->requestHeaders.remove("Content-Length");
        }
        else{ // if it is a multipart request

            QHash<QString,QString>::iterator i= this->requestHeaders.find("Content-Type");

            if(i != this->requestHeaders.end()){

                int p = i.value().indexOf("boundary=");

                if(p != -1){
                    QString cth = i.value();

                    QString bound = QString(cth.mid(cth.indexOf("boundary="),-1));
                    bound = bound.mid(9,bound.indexOf("\r\n"));
                    QString pr="";

                    QHash<QString,QString>::iterator it = this->queryItems.begin();

                    for(;it != this->queryItems.end();it++){

                        pr += ""+bound+"\r\n"+"Content-Disposition: form-data; name=\""+it.key()+"\"\r\n\r\n";
                        pr += (it.value())+"\r\n";
                        //pr += cUrlObject.escape(it.value())+"\r\n";


                    }
                    //__________
                    QString p = "";

                    QHash<QString,QString>::iterator i = this->queryItems.begin();

                    for(;i != this->queryItems.end();i++){

                        if( p != ""){
                            p += "&";
                        }
                        p += i.key()+"="+ cUrlObject.escape(i.value());
                    }
                    cUrlObject.requestOptions[CURLOPT_URL] = (toUrlEncoded(this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit()));
                    //__________

                    QByteArray collect = /*pr.toLocal8Bit() +*/ payload;


                    cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = collect;
                    cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();
                    this->requestHeaders.remove("Content-Length");
                    //this->requestHeaders.insert("Content-Length",QString::number(collect.size()));

                }

            }




        }
    }
    else{
        if(this->requestMethod != "get"){


            if(payload.size() == 0){
                cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = "";
                cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = 0;
            }
            else{

                QHash<QString,QString>::iterator i= this->requestHeaders.find("Content-Type");

                if(i != this->requestHeaders.end()){

                    int p = i.value().indexOf("boundary=");
                    if(p != -1){
                        QString cth = i.value();

                        QString bound = QString(cth.mid(cth.indexOf("boundary="),-1));
                        bound = bound.mid(9,bound.indexOf("\r\n"));

                        cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = payload;
                        cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = payload.size();
                    }
                    else{
                        cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = payload;
                        cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = payload.size();

                    }


                }
                else{
                    cUrlObject.requestOptions[CURLOPT_POSTFIELDS] = payload;
                    cUrlObject.requestOptions[CURLOPT_POSTFIELDSIZE] = payload.size();
                }
            }
        }


    }
    // set request headers



    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){


            QString hl = i.key() + ": "+i.value();

//            if(hl.contains("\"")){
//                hl = hl.replace("\"","\\\"");
//            }

//            hl = hl.replace("\r","");
//            hl = hl.replace("\n","");

            h << hl.toLocal8Bit();

        }

        cUrlObject.requestOptions[CURLOPT_HTTPHEADER] = h;
    }



}

#endif



void MSRequest::methodCharger(QNetworkRequest req, const QString &path){

    // fix warning message

    Q_UNUSED(path);


#ifdef CCROSS_LIB

    Q_UNUSED(req);

    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->setCURLOptions();
//qDebug() << this->cUrlObject.requestOptions[CURLOPT_POSTFIELDS];
    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        //this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());


        qint64 c = this->outFile->write(QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size()));
        c++;

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - CHARDGER-2 Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;



#endif

#ifndef CCROSS_LIB

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

//    connect(replySync,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(doDownloadProgress(qint64,qint64)));
    //connect(replySync,SIGNAL(readyRead()),this,SLOT(doReadyRead()));
//    connect(replySync,SIGNAL(finished()),this,SLOT(doRequestFinished()));

    //this->requesProcessed =true;
    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();

#endif
}



void MSRequest::raw_exec(const QString &reqestURL){

#ifdef CCROSS_LIB

    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->requestURL = QUrl::fromPercentEncoding(reqestURL.toLocal8Bit());
    this->requestMethod = "get";

    this->setCURLOptions();

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - RAW_EXEC Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;




    QtCUrl cUrl;
    QtCUrl::Options opt;

    opt[CURLOPT_URL] = toUrlEncoded(reqestURL);
    opt[CURLOPT_FOLLOWLOCATION] = 1;
    opt[CURLOPT_FAILONERROR] = 1;

    cUrl.exec(opt);

    if (cUrl.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(cUrl.buffer(), cUrl.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("Error: %1\nBuffer: %2").arg(cUrl.lastError().text()).arg(cUrl.errorBuffer()));
    }
#endif

#ifndef CCROSS_LIB
    QUrl r(reqestURL);

    QString s=QUrl::toPercentEncoding(r.path().toUtf8(),"(){}/","");

    QString url=r.scheme()+"://"+r.host()+""+ s;
    this->setRequestUrl(url);

    this->query->setQuery(r.query());
    this->setMethod("get");
    this->exec();
#endif
}



void MSRequest::post(const QByteArray &data){

#ifdef CCROSS_LIB

    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "");//application/x-www-form-urlencoded
        }
    }

    this->requestMethod = "post";
    this->setCURLOptions(data);

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - POST Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;


#endif

#ifndef CCROSS_LIB
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
#endif
}




void MSRequest::post(QIODevice *data){

#ifndef CCROSS_LIB
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
#endif

}


void MSRequest::download(const QString &url){

#ifdef CCROSS_LIB

    this->requestURL = QUrl::fromPercentEncoding(url.toLocal8Bit());

//    this->outFile= new QFile(path);
//    bool e=this->outFile->open(QIODevice::WriteOnly );

//    if(e == false){
//        this->replyError = QNetworkReply::NetworkError::UnknownContentError;
//        return;
//    }

    methodCharger(*this);

#endif

#ifndef CCROSS_LIB
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
#endif
}



void MSRequest::download(const QString &url, const QString &path){

#ifdef CCROSS_LIB

    this->requestURL = url.toLocal8Bit();

    this->outFile= new QFile(path);
    bool e=this->outFile->open(QIODevice::WriteOnly );

    if(e == false){
        this->replyError = QNetworkReply::NetworkError::UnknownContentError;
        return;
    }

    methodCharger(*this,path);

    this->outFile->close();

#endif

#ifndef CCROSS_LIB


    this->setUrl(QUrl(url));

    this->outFile= new QFile(path);
    bool e=this->outFile->open(QIODevice::WriteOnly );

    if(e == false){
        this->replyError = QNetworkReply::NetworkError::UnknownContentError;
        return;
    }




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
#endif

}



void MSRequest::syncDownloadWithGet( QString path){


    if(this->setQueryForDownload){
        this->url->setQuery(*this->query);
    }

    this->setUrl(*this->url);

    this->outFile= new QFile(path);
    bool e=this->outFile->open(QIODevice::WriteOnly );

    if(e == false){
        this->replyError = QNetworkReply::NetworkError::UnknownContentError;
        return;
    }




    QNetworkReply* replySync = this->manager->get(*this);
    this->currentReply = replySync;

    connect(replySync,SIGNAL(readyRead()),this,SLOT(doReadyRead()));
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();




    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();

}



void MSRequest::syncDownloadWithPost( QString path, QByteArray data){


    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    this->outFile= new QFile(path);
    bool e=this->outFile->open(QIODevice::WriteOnly );

    if(e == false){
        this->replyError = QNetworkReply::NetworkError::UnknownContentError;
        return;
    }




    QNetworkReply* replySync = this->manager->post(*this,data.data());
    this->currentReply = replySync;

    connect(replySync,SIGNAL(readyRead()),this,SLOT(doReadyRead()));
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    this->loop->exec();

    delete(replySync);
    //this->loop->exit(0);
    //loop.deleteLater();

}

void MSRequest::postMultipart(QHttpMultiPart *multipart){

    QNetworkReply* replySync;

    this->url->setQuery(*this->query);
    this->setUrl(*this->url);

    replySync=this->manager->post(*this,multipart);
    //multipart->setParent(replySync);

    //QEventLoop loop;
    connect(replySync, SIGNAL(finished()),this->loop, SLOT(quit()));
    loop->exec();

    //delete(replySync);

}




void MSRequest::put(const QByteArray &data){
#ifdef CCROSS_LIB



    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->requestMethod = "put";

    this->setCURLOptions(data);

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - PUT Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;



#endif

#ifndef CCROSS_LIB
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

#endif
}




#ifdef CCROSS_LIB
size_t readCallback(void *ptr, size_t size, size_t nmemb, void *stream){

    Q_UNUSED(size)
    return ((QFile*)stream)->read((char*)ptr,nmemb);
//  size_t retcode;
//  curl_off_t nread;

//  /* in real-world cases, this would probably get this data differently
//     as this fread() stuff is exactly what the library already would do
//     by default internally */
//  retcode = fread(ptr, size, nmemb, stream);

//  nread = (curl_off_t)retcode;


//  return retcode;
}
#endif

void MSRequest::put(QIODevice *data){

#ifdef CCROSS_LIB


    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->requestMethod = "put";

    this->setCURLOptions(data);

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;
    this->cUrlObject.requestOptions[CURLOPT_READFUNCTION].setValue( &readCallback);

//    int fh = ((QFile*)data)->handle();
//    FILE* fd = fdopen(fh,"rb");

    this->cUrlObject.requestOptions[CURLOPT_READDATA].setValue(data);// = qVariantFromValue((void*) data);//static_cast<void*> ((QFile*)data);

    this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - PUT IO Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;


#endif

#ifndef CCROSS_LIB
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
#endif
}



void MSRequest::deleteResource(){

#ifdef CCROSS_LIB


    if(this->requestHeaders.find("Content-Type") == this->requestHeaders.end()){

        if(!this->notUseContentType){

            //this->requestHeaders.insert("Content-Type", "application/x-www-form-urlencoded");
        }
    }

    this->requestMethod = "delete";

    this->setCURLOptions();

    this->cUrlObject.requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject.exec();

    if (this->cUrlObject.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(this->cUrlObject.buffer(), this->cUrlObject.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - DELETE Error: %1\nBuffer: %2").arg(this->cUrlObject.lastError().text()).arg(this->cUrlObject.errorBuffer()));
    }

    return;












    QtCUrl cUrl;
    QStringList headers;
    QtCUrl::Options opt;

    opt[CURLOPT_URL] = toUrlEncoded(this->url->url());

    opt[CURLOPT_CUSTOMREQUEST] = "DELETE";
    //opt[CURLOPT_UPLOAD] = 1;
    //opt[CURLOPT_INFILESIZE_LARGE] = data.size();

    opt[CURLOPT_POSTFIELDS] = this->toUrlEncoded(this->query->toString());
    opt[CURLOPT_POSTFIELDSIZE] = this->toUrlEncoded(this->query->toString()).size();

    if(!this->hasRawHeader("Content-Type")){

        if(!this->notUseContentType){
            headers << "Content-Type:    application/x-www-form-urlencoded";
        }
    }

    if(this->rawHeaderList().size() > 0){


        for(int i = 0; i < this->rawHeaderList().size(); i++){


            QString ss= QString(this->rawHeaderList().at(i)) + QString(":  ") + this->rawHeader(this->rawHeaderList().at(i));
            headers << ss;
        }

    }

    opt[CURLOPT_HTTPHEADER] = headers;


    cUrl.exec(opt);

    if (cUrl.lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;
        this->replyText = QByteArray(cUrl.buffer(), cUrl.buffer().size());

    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("Error: %1\nBuffer: %2").arg(cUrl.lastError().text()).arg(cUrl.errorBuffer()));
    }


#endif

#ifndef CCROSS_LIB
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

#endif
}




void MSRequest::exec(){

#ifdef CCROSS_LIB
    methodCharger(*this);

    return;
#endif

#ifndef CCROSS_LIB

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
#endif
}


void MSRequest::requestFinished(QNetworkReply *reply){


    //log("REQUEST FINISHED SLOT ");

    if(this->outFile!=nullptr){

        this->outFile->write(this->currentReply->readAll());
        this->outFile->close();
        delete(this->outFile);
        this->outFile=nullptr;
    }

    //this->lastReply=reply;
    this->replyHeaders=reply->rawHeaderPairs();
    this->replyText=reply->readAll();
    this->replyError=reply->error();
    this->replyErrorText=reply->errorString();
    this->replyAttribute=reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    this->replyURL=reply->url().toString();

#ifdef PRINT_DEBUG_INFO
    this->printDebugInfo_response(reply);
#endif

//    delete(reply);

}



void MSRequest::doDownloadProgress(qint64 avail, qint64 total){

   // fix warning message
    if(avail==total){
        return;
    }

    log("doDownloadProgress was entered");

}


void MSRequest::doReadyRead(){

    this->outFile->write(this->currentReply->readAll());

    //this->loop->exit(0);

    //log("doReadyRead was entered");
}




void MSRequest::doRequestFinished(){

//    QProcess process;
//    process.start("echo -e \"\a\" ");

    this->requesProcessed = false;
    this->loop->quit();
    log("Request finished");
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


#ifdef CCROSS_LIB




void MSRequest::MSsetCookieJar(MSNetworkCookieJar *cookie){

//    if(this->cookieJarObject != nullptr){

//        //this->cookieJarObject->deleteLater();
//        try{
//            delete this->cookieJarObject;
//        }catch(...){
//                int y=7;
//                y++;
//        }
//    }


    this->cookieJarObject=cookie;
    cUrlObject.requestOptions[CURLOPT_COOKIEJAR].setValue(cookie->getFileName());
    cUrlObject.requestOptions[CURLOPT_COOKIEFILE].setValue(cookie->getFileName());
    cUrlObject.requestOptions[CURLOPT_COOKIELIST].setValue(QString("RELOAD"));
}

MSNetworkCookieJar* MSRequest::getCookieJar(){

    return (this->cookieJarObject);
}


#else

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
    this->manager->setCookieJar(cookie);//this->cookieJar

}

#endif




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



bool MSRequest::cookieFromJSON(const QJsonObject &cookie)
{
    //cookie=QJsonObject();
    Q_UNUSED(cookie);
return true;
}


void MSRequest::setProxy(QNetworkProxy *proxy){

    this->manager->setProxy(*proxy);

}



void MSRequest::log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%T - ",timeinfo);

        mes = QString(buffer)+"MSRequest : "+mes+" \n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

//    string ns="echo "+mes+" >> /tmp/ccfw.log ";
//    system(ns.c_str());
    return;
}
