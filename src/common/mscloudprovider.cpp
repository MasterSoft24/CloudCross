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

void MSCloudProvider::saveTokenFile(QString path){
    // fix warning message
    path=path;
    return;
}

bool MSCloudProvider::loadTokenFile(QString path){
    // fix warning message
    path=path;
    return false;
}

bool MSCloudProvider::refreshToken(){
    return false;
}


QString MSCloudProvider::fileChecksum(QString &fileName, QCryptographicHash::Algorithm hashAlgorithm){

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
qint64 MSCloudProvider::toMilliseconds(QDateTime dateTime, bool isUTC){

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
qint64 MSCloudProvider::toMilliseconds(QString dateTimeString, bool isUTC){

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


void MSCloudProvider::setFlag(QString flagName, bool flagVal){

    this->flags.insert(flagName,flagVal);

}


bool MSCloudProvider::getFlag(QString flagName){

    QHash<QString,bool>::iterator it=this->flags.find(flagName);
    if(it != this->flags.end()){
        return it.value();
    }
    else{
        return false;
    }
}

QString MSCloudProvider::getOption(QString optionName){

    QHash<QString,QString>::iterator it=this->options.find(optionName);
    if(it != this->options.end()){
        return it.value();
    }
    else{
        return QString();
    }
}


bool MSCloudProvider::local_writeFileContent(QString filePath, MSRequest* req){

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

void MSCloudProvider::local_createDirectory(QString path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);

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

    int u=7;

    this->oauthListenerSocket=this->oauthListener->nextPendingConnection();
    //QHostAddress pa=this->oauthListenerSocket->co;
    connect(this->oauthListenerSocket,SIGNAL(readyRead()),this, SLOT(onDataRecieved()));
}


void MSCloudProvider::onDataRecieved(){


    QTextStream os(this->oauthListenerSocket);
        os.setAutoDetectUnicode(true);

       // qDebug() << this->oauthListenerSocket->readAll()+"\n\r";
        QString c=this->oauthListenerSocket->readLine();


        int cb=c.indexOf("code=");
        int ce=0;
        QString code;

        if(cb != -1){

            ce=c.indexOf(" ",cb);
            code=c.mid(cb+5,ce-cb-5);

	    this->oauthListenerSocket->close();

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
    this->oauthListener->listen(QHostAddress("127.0.0.1"),port);

}


bool MSCloudProvider::stopListener(){

    //this->oauthListener->pauseAccepting();
    this->oauthListener->close();
}





