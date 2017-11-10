#include "mshttprequest.h"

typedef size_t(*hdr_callback)(char *buffer, size_t size,size_t nitems, void *userdata);



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MSHttpRequest::MSHttpRequest(MSNetworkProxy* proxy = nullptr)
{

    this->dataStreamType = dataStreamTypes::DS_None;

    this->cUrlObject = new QtCUrl();

    this->cUrlObject->outFile = nullptr;
    this->cUrlObject->inpFile = nullptr;
    this->cookieJarObject = nullptr;
    this->cUrlObject->replyHeaders.clear();

    this->proxy = nullptr;

    this->cUrlObject->payloadChunkSize =0;

    if(proxy !=nullptr){
        this->setProxy(proxy);
    }

}

//==============================================================================================

MSHttpRequest::~MSHttpRequest(){

    if(this->cUrlObject->outFile != nullptr){

        delete(this->cUrlObject->outFile);
    }

    if(this->cookieJarObject != nullptr){
        //delete(this->cookieJarObject);
    }


    delete(this->cUrlObject);
}

//==============================================================================================

void MSHttpRequest::setProxy(MSNetworkProxy *proxy){

    cUrlObject->requestOptions[CURLOPT_PROXY] = (proxy->getProxyString());
    this->proxy = proxy;
}

//==============================================================================================

void MSHttpRequest::disableProxy(){

     if(cUrlObject->requestOptions.find(CURLOPT_PROXY) != cUrlObject->requestOptions.end()){
         cUrlObject->requestOptions.remove(CURLOPT_PROXY);

         this->proxy = nullptr;
     }
}

//==============================================================================================

bool MSHttpRequest::setMethod(const QString &method){

    if((method.toLower() == QStringLiteral("post"))||(method.toLower() == QStringLiteral("get"))||(method.toLower() == QStringLiteral("put"))||(method.toLower() == QStringLiteral("delete"))){
        this->requestMethod=method.toUpper();
        return true;
    }
    else{
        return false;
    }
}

//==============================================================================================

void MSHttpRequest::setPayloadChunkData(qint64 size, quint64 pos){
    this->cUrlObject->payloadChunkSize = size;
    this->cUrlObject->payloadFilePosition = pos;
}

//==============================================================================================

void MSHttpRequest::setRequestUrl(const QString &url){

     this->requestURL = /*QUrl::fromPercentEncoding*/(url/*.toLocal8Bit()*/);
}
//==============================================================================================

void MSHttpRequest::addQueryItem(const QString &itemName, const QString &itemValue){

    this->queryItems.insert(itemName,itemValue);
}
//==============================================================================================

void MSHttpRequest::addHeader(const QString &headerName, const QString &headerValue){

    this->requestHeaders.insert(headerName,headerValue);
}
//==============================================================================================

void MSHttpRequest::addHeader(const QByteArray &headerName, const QByteArray &headerValue){

    this->requestHeaders.insert(headerName,headerValue);
}
//==============================================================================================

QString MSHttpRequest::getReplyHeader(const QByteArray &headerName){

    QList<QPair<QByteArray,QByteArray>> hl = this->cUrlObject->replyHeaders;

    for(int i=0;i<hl.size();i++){

        if(hl[i].first == headerName){

            return hl[i].second;
        }
    }

    return QString();
}
//==============================================================================================

void MSHttpRequest::setOutputFile(const QString &fileName){

    this->cUrlObject->outFile = new QFile(fileName);
//    this->cUrlObject->outFile->open(QIODevice::WriteOnly);

}
//==============================================================================================

void MSHttpRequest::setInputDataStream(const QByteArray &data){

    this->dataStreamType = dataStreamTypes::DS_ByteArray;
    this->dataStreamByteArray = data;
}
//==============================================================================================

void MSHttpRequest::setInputDataStream(QFile *data){

    this->dataStreamType = dataStreamTypes::DS_File;
    this->dataStreamFile = data->fileName();
}
//==============================================================================================

void MSHttpRequest::setInputDataStream(QMultiBuffer *data){

    this->dataStreamType = dataStreamTypes::DS_MultiBuffer;

    for(int i = 0;i < data->items.size(); i++){

        if(data->items[i].fileName == ""){
            QIODevice* dd = ((QIODevice*)qvariant_cast<QIODevice*> (data->items[i].slot));
            dd->open(QIODevice::ReadOnly);
            QByteArray b = dd->readAll();
            this->dataStreamMultiBuffer.append(QPair<dataStreamTypes,QByteArray>(dataStreamTypes::DS_ByteArray,b));
        }
        else{
            QFile* f = qvariant_cast<QFile*> (data->items[i].slot);
            this->dataStreamMultiBuffer.append(QPair<dataStreamTypes,QByteArray>(dataStreamTypes::DS_File,f->fileName().toStdString().c_str()));
        }


    }

    return;
}

//==============================================================================================

QString MSHttpRequest::replyURL(){
    return this->cUrlObject->replyURL;
}
//==============================================================================================

void MSHttpRequest::MSsetCookieJar(MSNetworkCookieJar *cookie){

    this->cookieJarObject=cookie;
    cUrlObject->requestOptions[CURLOPT_COOKIEJAR].setValue(cookie->getFileName());
    cUrlObject->requestOptions[CURLOPT_COOKIEFILE].setValue(cookie->getFileName());
    cUrlObject->requestOptions[CURLOPT_COOKIELIST].setValue(QString("RELOAD"));

}
//==============================================================================================

MSNetworkCookieJar *MSHttpRequest::getCookieJar(){

     return (this->cookieJarObject);
}
//==============================================================================================

void MSHttpRequest::post(const QByteArray &data){

    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);

    cUrlObject->requestOptions[CURLOPT_CUSTOMREQUEST] ="POST";
    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL).toLocal8Bit();

    if(data.size() == 0){
        QString p = "";

        QHash<QString,QString>::iterator i = this->queryItems.begin();

        for(;i != this->queryItems.end();i++){

            if( p != ""){
                p += "&";
            }
            p += i.key()+"="+ cUrlObject->escape(i.value());
        }

//        if(this->requestMethod.toUpper() == "POST"){


            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit());
            cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = ((p.toLocal8Bit()));
            cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = ((p.toLocal8Bit())).size() ;
//        }
//        else{

//            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit())+"?"+p.toLocal8Bit();

//        }

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
                    //pr += cUrlObject->escape(it.value())+"\r\n";


                }
                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

                QByteArray collect = /*pr.toLocal8Bit() +*/ data;


                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();
                this->requestHeaders.remove("Content-Length");
                //this->requestHeaders.insert("Content-Length",QString::number(collect.size()));

            }
            else{ // if it is a post with payload

                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

                QByteArray collect = /*pr.toLocal8Bit() +*/ data;



                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();


            }

        }
    }

    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }
}
//==============================================================================================

void MSHttpRequest::post(QIODevice *data){


    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);



    cUrlObject->requestOptions[CURLOPT_CUSTOMREQUEST] ="POST";
    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL).toLocal8Bit();

    if(data->size() == 0){
        QString p = "";

        QHash<QString,QString>::iterator i = this->queryItems.begin();

        for(;i != this->queryItems.end();i++){

            if( p != ""){
                p += "&";
            }
            p += i.key()+"="+ /*cUrlObject->escape*/(i.value()); // 26.10 not sent mailru update file
        }

//        if(this->requestMethod.toUpper() == "POST"){

            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit());
            cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = ((p.toLocal8Bit()));
            cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = ((p.toLocal8Bit())).size() ;
//        }
//        else{

//            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit())+"?"+p.toLocal8Bit();

//        }

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
                    //pr += cUrlObject->escape(it.value())+"\r\n";


                }
                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

//                QByteArray collect = /*pr.toLocal8Bit() +*/ data;

                cUrlObject->requestOptions[CURLOPT_POST] = 1;
                cUrlObject->requestOptions[CURLOPT_READDATA] = (qlonglong)(this->cUrlObject);

                if(this->cUrlObject->payloadChunkSize != 0){
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE] = this->cUrlObject->payloadChunkSize;
                }
                else{
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE] = data->size();
                }

                cUrlObject->inpFile = data;

//                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
//                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();
//                this->requestHeaders.remove("Content-Length");
                //this->requestHeaders.insert("Content-Length",QString::number(collect.size()));

            }
            else{ // if it is a post with payload

                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }


                if(p == QStringLiteral("")){
                    cUrlObject->requestOptions[CURLOPT_URL] = this->requestURL.toLocal8Bit();
                }
                else{
                    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                }

                //__________

//                QByteArray collect = /*pr.toLocal8Bit() +*/ data;

                cUrlObject->requestOptions[CURLOPT_POST] = 1;
                cUrlObject->requestOptions[CURLOPT_UPLOAD] = 1;
                cUrlObject->requestOptions[CURLOPT_READDATA] = (qlonglong)(this->cUrlObject);

                if(this->cUrlObject->payloadChunkSize != 0){
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE].setValue(this->cUrlObject->payloadChunkSize);// = this->payloadChunkSize;
                }
                else{
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE].setValue(data->size());// = data->size();
                }
//                cUrlObject->requestOptions[CURLOPT_INFILESIZE] = data->size();
                cUrlObject->inpFile = data;

//                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
//                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();


            }

        }
    }



    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }


}
//==============================================================================================

void MSHttpRequest::put(const QByteArray &data){

    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);

    cUrlObject->requestOptions[CURLOPT_CUSTOMREQUEST] ="PUT";
//    cUrlObject->requestOptions[CURLOPT_PUT] = 1;
    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL).toLocal8Bit();

    if(data.size() == 0){
        QString p = "";

        QHash<QString,QString>::iterator i = this->queryItems.begin();

        for(;i != this->queryItems.end();i++){

            if( p != ""){
                p += "&";
            }
            p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
        }

//        if(this->requestMethod.toUpper() == "POST"){

//            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit());
//            cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = ((p.toLocal8Bit()));
//            cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = ((p.toLocal8Bit())).size() ;
//        }
//        else{

            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit())+"?"+p.toLocal8Bit();

//        }

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
                    //pr += cUrlObject->escape(it.value())+"\r\n";


                }
                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

                QByteArray collect = /*pr.toLocal8Bit() +*/ data;


                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();
                this->requestHeaders.remove("Content-Length");
                //this->requestHeaders.insert("Content-Length",QString::number(collect.size()));

            }
            else{ // if it is a post with payload

                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

                QByteArray collect = /*pr.toLocal8Bit() +*/ data;


                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();


            }

        }
    }

    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }

}
//==============================================================================================

void MSHttpRequest::put(QIODevice *data){


    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);



    cUrlObject->requestOptions[CURLOPT_CUSTOMREQUEST] ="PUT";
    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL).toLocal8Bit();

    if(data->size() == 0){
        QString p = "";

        QHash<QString,QString>::iterator i = this->queryItems.begin();

        for(;i != this->queryItems.end();i++){

            if( p != ""){
                p += "&";
            }
            p += i.key()+"="+ cUrlObject->escape(i.value());
        }

//        if(this->requestMethod.toUpper() == "POST"){

//            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit());
//            cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = ((p.toLocal8Bit()));
//            cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = ((p.toLocal8Bit())).size() ;
//        }
//        else{

            cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit())+"?"+p.toLocal8Bit();

//        }

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
                    //pr += cUrlObject->escape(it.value())+"\r\n";


                }
                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }
                cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                //__________

//                QByteArray collect = /*pr.toLocal8Bit() +*/ data;

                cUrlObject->requestOptions[CURLOPT_PUT] = 1;
                cUrlObject->requestOptions[CURLOPT_READDATA] = (qlonglong)(this->cUrlObject);
                cUrlObject->requestOptions[CURLOPT_INFILESIZE] = data->size();
                cUrlObject->inpFile = data;

//                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
//                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();
//                this->requestHeaders.remove("Content-Length");
                //this->requestHeaders.insert("Content-Length",QString::number(collect.size()));

            }
            else{ // if it is a post with payload

                //__________
                QString p = "";

                QHash<QString,QString>::iterator i = this->queryItems.begin();

                for(;i != this->queryItems.end();i++){

                    if( p != ""){
                        p += "&";
                    }
                    p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
                }

                if(p == QStringLiteral("")){
                    cUrlObject->requestOptions[CURLOPT_URL] = this->requestURL.toLocal8Bit();
                }
                else{
                    cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
                }


                //__________

//                QByteArray collect = /*pr.toLocal8Bit() +*/ data;

                cUrlObject->requestOptions[CURLOPT_PUT] = 1;
                cUrlObject->requestOptions[CURLOPT_READDATA] = (qlonglong)(this->cUrlObject);
                if(this->cUrlObject->payloadChunkSize != 0){
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE] = this->cUrlObject->payloadChunkSize;
                }
                else{
                    cUrlObject->requestOptions[CURLOPT_INFILESIZE_LARGE] = data->size();
                }

                cUrlObject->inpFile = data;

//                cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = collect;
//                cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = collect.size();


            }

        }
    }

    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }


}
//==============================================================================================

void MSHttpRequest::deleteResource(){

    //this->setCURLOptions();
    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_CUSTOMREQUEST] ="DELETE";
    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);

    QString p = "";

    QHash<QString,QString>::iterator i = this->queryItems.begin();

    for(;i != this->queryItems.end();i++){

        if( p != ""){
            p += "&";
        }
        p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
    }

    if(p!= ""){
        cUrlObject->requestOptions[CURLOPT_URL] = (this->requestURL.toLocal8Bit()+"?"+p.toLocal8Bit());
    }
    else{
        cUrlObject->requestOptions[CURLOPT_URL] = this->requestURL.toLocal8Bit();
    }


//    cUrlObject->requestOptions[CURLOPT_POSTFIELDS] = "";
//    cUrlObject->requestOptions[CURLOPT_POSTFIELDSIZE] = 0;

    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = 1;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - DELETE Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }


}
//==============================================================================================

void MSHttpRequest::get(){

    //this->setCURLOptions();
    this->cUrlObject->requestOptions[CURLOPT_USERAGENT] = "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:15.0) Gecko/20100101 Firefox/15.0.1";

    //hdr_callback hcb = header_callback;

    cUrlObject->requestOptions[CURLOPT_HEADERDATA] = (qlonglong)(this->cUrlObject);
    cUrlObject->requestOptions[CURLOPT_WRITEDATA] = (qlonglong)(this->cUrlObject);


    QString p = "";

    if(this->queryItems.size() > 0){

        QHash<QString,QString>::iterator i = this->queryItems.begin();

        for(;i != this->queryItems.end();i++){

            if( p != ""){
                p += "&";
            }
            p += i.key()+"="+ /*cUrlObject->escape*/(i.value());
        }

        cUrlObject->requestOptions[CURLOPT_URL] = this->requestURL.toLocal8Bit() + "?" + p.toLocal8Bit();//toUrlEncoded(this->requestURL.toLocal8Bit())+"?" + toUrlEncoded(p.toLocal8Bit());

    }
    else{
        cUrlObject->requestOptions[CURLOPT_URL] = /*cUrlObject->escape*/(this->requestURL.toLocal8Bit());//.toLocal8Bit();
        //qDebug()<< this->requestURL;
    }

    // set request headers
    if(this->requestHeaders.size() > 0){

        QStringList h;
        QHash<QString,QString>::iterator i = this->requestHeaders.begin();

        for(;i != this->requestHeaders.end();i++){

            QString hl = i.key() + ": "+i.value();

            h << hl.toLocal8Bit();

        }

        cUrlObject->requestOptions[CURLOPT_HTTPHEADER] = h;
    }

    this->cUrlObject->requestOptions[CURLOPT_FOLLOWLOCATION] = true;


    QString r = this->cUrlObject->exec();

    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }
        else{
            //this->outFile->write(QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size()));
            //this->outFile->close();
        }


    }
    else {
        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << (QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }

}
//==============================================================================================

void MSHttpRequest::exec(){

    QByteArray d;
    QDataStream ds(&d,QIODevice::WriteOnly);


    ds << this->requestMethod;
    ds << this->requestURL;
    ds << this->queryItems;
    ds << this->requestHeaders;

    ds << this->cUrlObject->payloadChunkSize;
    ds << this->cUrlObject->payloadFilePosition;

    if( this->proxy != nullptr){
        QString pst = this->proxy->getProxyString();
        ds << this->proxy->getProxyString().toLocal8Bit();
    }
    else{
        ds << QByteArray("NO_PROXY");
    }

    if(this->cookieJarObject == nullptr){
        ds << QByteArray("NO_COOKIE");
    }
    else{

        this->cookieJarObject->cookieFile->seek(0);
        QByteArray cba = this->cookieJarObject->cookieFile->readAll();
        if(cba.size() == 0){
            ds << QByteArray("EMPTY_COOKIE");
        }
        else{
            ds << cba;
        }

    }

    if(this->cUrlObject->outFile == nullptr){
        ds << "NO_OUTFILE";
    }
    else{
        ds << this->cUrlObject->outFile->fileName().toStdString().c_str();
    }


    ds << (quint8)(this->dataStreamType);

    if(this->dataStreamType == MSHttpRequest::dataStreamTypes::DS_ByteArray){
        ds << this->dataStreamByteArray;
    }

    if(this->dataStreamType == MSHttpRequest::dataStreamTypes::DS_File){
        ds << this->dataStreamFile.toStdString().c_str();
    }

    if(this->dataStreamType == MSHttpRequest::dataStreamTypes::DS_MultiBuffer){

        ds << this->dataStreamMultiBuffer.size();

        for(int i =0; i< this->dataStreamMultiBuffer.size(); i++){

            ds << this->dataStreamMultiBuffer[i].first;
            ds << this->dataStreamMultiBuffer[i].second;

        }
    }


    QStringList p;

        QProcess *exe = new QProcess();

//        connect(exe, SIGNAL(readyReadStandardOutput()), this, SLOT(readExecutorOutput()));
        connect(exe, SIGNAL(finished(int)), this, SLOT(readExecutorOutput()));

        exe->start("ccross-curl",p);

        Q_PID exe_pid = exe->pid();
        if(exe_pid <= 0){

            // to do something for not executor found cause
        }

//qStdOut() << d.toBase64();
        exe->write( d.toBase64());
        exe->closeWriteChannel();
        exe->waitForFinished(999999999);

        delete(exe);

    return ;


}
//==============================================================================================

void MSHttpRequest::raw_exec(const QString &reqestURL){

    this->requestURL = reqestURL;
    this->requestMethod = "get";
    this->get();

}
//==============================================================================================

void MSHttpRequest::download(const QString &url){
Q_UNUSED(url);
}
//==============================================================================================

void MSHttpRequest::download(const QString &url, const QString &path){
Q_UNUSED(url);
Q_UNUSED(path)    ;
}
//==============================================================================================

QString MSHttpRequest::readReplyText(){
    return this->replyText;
}

//==============================================================================================

bool MSHttpRequest::replyOK(){

    if(this->replyError == QNetworkReply::NetworkError::NoError){
        return true;
    }
    else{
        return false;
    }
}

//==============================================================================================

void MSHttpRequest::printReplyError(){

    qStdOut() << this->replyErrorText << endl;
}

//==============================================================================================


void MSHttpRequest::readExecutorOutput(){ // read data from CurlExecutor and place it to current object

    QObject* obj = sender();
    QProcess* po = (QProcess*)obj;


    QByteArray b64 = po->readAllStandardOutput();
    QByteArray b64_decoded = QByteArray::fromBase64(b64);
    //qint32 bzs=b64_decoded.size();



    QDataStream ds(QByteArray::fromBase64(b64));

    QByteArray b,e;
    uint curlcode;


    ds >> this->cUrlObject->replyHeaders;
    ds >> b; // _buffer

    this->cUrlObject->_buffer.append(b.toStdString().c_str());
    ds >> curlcode; //
    this->cUrlObject->_lastCode._code = static_cast<CURLcode>(curlcode); ;

    ds >> e; // _errorBuffer

    uint ebsz = e.size();

    this->cUrlObject->_errorBuffer = (char*) malloc(ebsz);
    memcpy(this->cUrlObject->_errorBuffer, e.data(),ebsz);

    QByteArray ru;
    ds >> ru;
    this->cUrlObject->replyURL=QString(ru);

    QByteArray cookie;
    ds >> cookie;
    if((QString(cookie) != "NO_COOKIE")&&(QString(cookie) != "")&&(this->cookieJarObject != nullptr)){

        // set cookie object

        this->cookieJarObject->cookieFile->seek(0);
        this->cookieJarObject->cookieFile->write(cookie);
        this->cookieJarObject->cookieFile->flush();
    }


    if (this->cUrlObject->lastError().isOk()) {

        this->replyError = QNetworkReply::NetworkError::NoError;

        if(this->cUrlObject->outFile == nullptr){
            this->replyText = QByteArray(this->cUrlObject->buffer(), this->cUrlObject->buffer().size());
        }

    }
    else {

        this->replyErrorText = this->cUrlObject->lastError().text() + this->cUrlObject->errorBuffer();

        this->replyError = QNetworkReply::NetworkError::ContentGoneError;
        qDebug() << this->replyErrorText;//(QString("MSRequest - GET Error: %1\nBuffer: %2").arg(this->cUrlObject->lastError().text()).arg(this->cUrlObject->errorBuffer()));
    }


   // qDebug() << rd;

}
