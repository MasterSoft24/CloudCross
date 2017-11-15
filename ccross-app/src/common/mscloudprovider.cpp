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

#include "include/mscloudprovider.h"





MSCloudProvider::MSCloudProvider(QObject *parent)
    : QObject( parent )
{
    this->lastSyncTime=0;
    this->strategy=MSCloudProvider::SyncStrategy::PreferLocal;// by default

    // set flags to default
    this->flags.insert("dryRun",false);
    this->flags.insert("useInclude",false);
    this->flags.insert("noHidden",false);
    this->flags.insert("newRev",false);

    this->oauthListener = new QTcpServer();
    connect(this->oauthListener, SIGNAL(newConnection()), this, SLOT(onIncomingConnection()));
   // this->oauthListener->connect(SIGNAL(newConnection()),this, SLOT(onIncomingConnection()));

    this->proxyServer=0;
}

//=======================================================================================

bool MSCloudProvider::setProxyServer(const QString &type, const QString &proxy)
{
    QStringList pa=proxy.split(':');

    if(pa.size()!=2){
        return false;
    }

    QString addr=pa[0];
    qint32 port= pa[1].toInt();

//    this->proxyServer = new QNetworkProxy();
    this->proxyServer = new MSNetworkProxy();

    this->proxyServer->setHostName(addr);
    this->proxyServer->setPort(port);

    if(type == "http"){
        this->proxyServer->setType(MSNetworkProxy::HttpProxy);
    }
    if(type == "socks5"){
        this->proxyServer->setType(MSNetworkProxy::Socks5Proxy);
    }

    return true;
}

//=======================================================================================

bool MSCloudProvider::filterServiceFileNames(const QString &path){// return false if input path is service filename

    QString reg = "ccrosstemp.*|"+this->trashFileName+"*|"+this->tokenFileName+"|"+this->stateFileName+"|.include|.exclude|~";
    QRegExp regex(reg);
    int ind = regex.indexIn(path);

    if(ind != -1){
        return false;
    }
    return true;

}

//=======================================================================================

bool MSCloudProvider::filterIncludeFileNames(const QString &path){// return false if input path contain one of include mask

    if(this->includeList==""){
        return true;
    }
    QString filterType = this->getOption("filter-type");
    bool isBegin=false;
    if(filterType == "regexp"){
        // catch paths with  beginning masks from include/exclude lists
        QRegExp regex3(path);
        regex3.setPatternSyntax(QRegExp::RegExp);
        regex3.isValid();
        int m1 = regex3.indexIn(this->includeList);

        if(m1 != -1){
            if((this->includeList.mid(m1-1,1)=="|") ||(m1==0)){
                isBegin=true;
            }
        }
        QRegExp regex2(this->includeList);
        regex2.setPatternSyntax(QRegExp::RegExp);
        regex2.isValid();

        int m = regex2.indexIn(path);
        if(m != -1){
            return false;
        }
        else{
            if(isBegin){
                return false;
            }
            else{
                return true;
            }
        }
    }
    else{
        QStringList filters = this->includeList.split('|');
        //qDebug() << filters;
        //qDebug() << path;
        for(QString &filt : filters){
            QRegExp regex2(filt);
            regex2.setPatternSyntax(QRegExp::Wildcard);
            regex2.isValid();
            int m = regex2.indexIn(path);

            if(m != -1){
                //qDebug() << path << "     match";
                return false;
            }
        }
        return true;
    }
}

//=======================================================================================

bool MSCloudProvider::filterExcludeFileNames(const QString &path){// return false if input path contain one of exclude mask

    if(this->excludeList==""){
        return true;
    }
    QString filterType = this->getOption("filter-type");
    bool isBegin=false;

    if(filterType == "regexp"){
        // catch paths with  beginning masks from include/exclude lists
        QRegExp regex3(path);
        regex3.setPatternSyntax(QRegExp::RegExp);
        regex3.isValid();
        int m1 = regex3.indexIn(this->excludeList);

        if(m1 != -1){
            if((this->excludeList.mid(m1-1,1)=="|")||(m1==0)){
                isBegin=true;
            }
        }
        QRegExp regex2(this->excludeList);
        regex2.setPatternSyntax(QRegExp::RegExp);
        regex2.isValid();

        int m = regex2.indexIn(path);
        if(m != -1){
            return false;
        }
        else{
            if(isBegin){
                return false;
            }
            else{
                return true;
            }
        }
    }
    else{
        QStringList filters = this->excludeList.split('|');
        for(QString &filt : filters){
            QRegExp regex2(filt);
            regex2.setPatternSyntax(QRegExp::Wildcard);
            regex2.isValid();
            int m = regex2.indexIn(path);

            if(m != -1){
                return false;
            }
        }
        return true;
    }
}

void MSCloudProvider::saveTokenFile(const QString &path){
    // fix warning message
    Q_UNUSED(path);
    return;
}

bool MSCloudProvider::loadTokenFile(const QString &path){
    // fix warning message
    Q_UNUSED(path);
    return false;
}

bool MSCloudProvider::refreshToken(){
    return false;
}


QString MSCloudProvider::fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm){

    QFile f(fileName);

    if (f.open(QFile::ReadOnly)){

            QCryptographicHash hash(hashAlgorithm);

            if (hash.addData(&f)){

                return QString(hash.result().toHex());

            }
    }
    return QString();
}

// convert to milliseconds in UTC timezone
qint64 MSCloudProvider::toMilliseconds( QDateTime dateTime, bool isUTC){

    if(!isUTC){// dateTime currently in UTC, need convert

        QDateTime dateTime1=QDateTime(dateTime);
        qint64 delta;


        dateTime.setTimeSpec(Qt::UTC);
        dateTime=dateTime.toLocalTime();

        delta=dateTime.toMSecsSinceEpoch() - dateTime1.toMSecsSinceEpoch();

        QDateTime dd=QDateTime::fromMSecsSinceEpoch(dateTime1.toMSecsSinceEpoch() - delta);

        return dateTime1.toMSecsSinceEpoch() - delta;

    }
    else{
        return dateTime.toMSecsSinceEpoch();
    }

}


// convert to milliseconds in UTC timezone
qint64 MSCloudProvider::toMilliseconds(const QString &dateTimeString, bool isUTC){

    if(!isUTC){

        QDateTime d=QDateTime::fromString(dateTimeString,Qt::DateFormat::ISODate);
        QDateTime d1=QDateTime(d);

        qint64 delta;

        d.setTimeSpec(Qt::UTC);
        d=d.toLocalTime();

        delta= d.toMSecsSinceEpoch() - d1.toMSecsSinceEpoch();

        QDateTime dd=QDateTime::fromMSecsSinceEpoch(d1.toMSecsSinceEpoch() - delta);

        return  d1.toMSecsSinceEpoch() - delta;
    }
    else{

        QDateTime d=QDateTime::fromString(dateTimeString,Qt::DateFormat::ISODate);
        return d.toMSecsSinceEpoch();
    }

}


void MSCloudProvider::setFlag(const QString &flagName, bool flagVal){

    this->flags.insert(flagName,flagVal);

}


bool MSCloudProvider::getFlag(const QString &flagName){

    QHash<QString,bool>::iterator it=this->flags.find(flagName);
    if(it != this->flags.end()){
        return it.value();
    }
    else{
        return false;
    }
}

QString MSCloudProvider::getOption(const QString &optionName){

    QHash<QString,QString>::iterator it=this->options.find(optionName);
    if(it != this->options.end()){
        return it.value();
    }
    else{
        return QString();
    }
}



bool MSCloudProvider::local_writeFileContent(const QString &filePath, MSHttpRequest* req){

    if(req->replyError!= QNetworkReply::NetworkError::NoError){
        qStdOut()<< "Request error. ";
        req->printReplyError();
        return false;
    }


    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly )){

        QFileInfo d=QFileInfo(filePath);
        QString p=d.absolutePath();
        this->local_createDirectory(p);

        if(!file.open(QIODevice::WriteOnly )){
            return false;
        }
    }

    QDataStream outk(&file);

    QByteArray ba;
    ba.append(req->readReplyText());

    int sz=ba.size();


    outk.writeRawData(ba.data(),sz) ;

    file.close();
    return true;
}

//=======================================================================================

void MSCloudProvider::local_createDirectory(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);

}


//=======================================================================================

void MSCloudProvider::local_actualizeTempFile(QString tempPath){

    QFile temp(tempPath);
    QString realPath = tempPath.replace(CCROSS_TMP_PREFIX,"");
    QFile real(realPath);

    if(real.exists()){
        real.remove();
    }

    temp.rename(realPath);
}


//=======================================================================================

QString MSCloudProvider::generateRandom(int count){

    int Low=0x41;
    int High=0x5a;
    QDateTime d;

    qsrand(d.currentDateTime().toMSecsSinceEpoch());

    QString token="";


    for(int i=0; i<count;i++){
        qint8 d=qrand() % ((High + 1) - Low) + Low;

        if(d == 92){
           token+="\\"; // экранируем символ
        }
        else{
            token+=QChar(d);
        }
    }

    return token;

}




void MSCloudProvider::onIncomingConnection(){


    this->oauthListenerSocket = this->oauthListener->nextPendingConnection();
    //QHostAddress pa=this->oauthListenerSocket->co;
    connect(this->oauthListenerSocket,SIGNAL(readyRead()),this, SLOT(onDataRecieved()));

}


void MSCloudProvider::onDataRecieved(){


    QTextStream os(this->oauthListenerSocket);
        os.setAutoDetectUnicode(true);

       // qDebug() << this->oauthListenerSocket->readAll()+"\n\r";
        QString c=this->oauthListenerSocket->readAll();//readLine();

        int cb=c.indexOf("code=");
        int ce=0;


        QString code;

        if(cb != -1){

            ce=c.indexOf(" ",cb);
            code=c.mid(cb+5,ce-cb-5);

            this->oauthListenerSocket->write("HTTP/1.1 302 Found\r\nLocation: https://cloudcross.mastersoft24.ru/authcomplete.html");
            //this->oauthListenerSocket->write("HTTP/1.0 200 OK\r\n<html><body><h1>Auth complete</h1></body></html>\r\n");
            this->oauthListenerSocket->waitForBytesWritten();

            //this->oauthListenerSocket->close();

            this->stopListener();

            emit oAuthCodeRecived(code,this);
        }
        else{

            this->oauthListenerSocket->close();

            this->stopListener();

            emit oAuthError("",this);

        }


}


bool MSCloudProvider::startListener(int port){

   // this->oauthListener->resumeAccepting();
    return this->oauthListener->listen(QHostAddress("127.0.0.1"),port);


}


bool MSCloudProvider::stopListener(){

    //this->oauthListener->pauseAccepting();
    this->oauthListener->close();

    return true;
}





