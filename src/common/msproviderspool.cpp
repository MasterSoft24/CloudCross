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

#include "include/msproviderspool.h"
#include <QDir>


MSProvidersPool::MSProvidersPool()
{
    this->getCurrentPath();
    this->strategy=MSCloudProvider::SyncStrategy::PreferLocal;
    this->proxyAddrString="";
    this->proxyTypeString="";

}


QString MSProvidersPool::generateRandom(int count){

    int Low=35;
    int High=127;
    qsrand(QDateTime::currentMSecsSinceEpoch());
    //qsrand(qrand());

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

void MSProvidersPool::addProvider(MSCloudProvider *provider,bool statelessMode){

    MSCloudProvider* cp=this->getProvider(provider->providerName);
    if(cp==NULL){
        provider->currentPath=this->currentPath;
        provider->workPath=this->workPath;
        provider->credentialsPath = this->workPath; // WARNING
        provider->strategy=this->strategy;

        QHash<QString,bool>::iterator it=this->flags.begin();
        while(it != this->flags.end()){

            provider->flags.insert(it.key(),it.value());
            it++;
        }

        QHash<QString,QString>::iterator ito=this->options.begin();
        while(ito != this->options.end()){

            provider->options.insert(ito.key(),ito.value());
            ito++;
        }

        if(!statelessMode){
            provider->loadStateFile();
        }

        this->pool.append(provider);

        return;
    }
}

void MSProvidersPool::getCurrentPath(){
    this->currentPath=QDir::currentPath();
    this->workPath=this->currentPath;
    this->credentialsPath = this->workPath; // WARNING
}


void MSProvidersPool::setWorkPath(const QString &path){

#ifdef Q_OS_WIN
   path=path.replace("\\","/") ;
#endif
    this->workPath=path;

}

MSCloudProvider* MSProvidersPool::getProvider(const QString &providerName){
    for(int i=0; i< this->pool.size();i++){

        if(this->pool[i]->providerName==providerName){
            return (MSCloudProvider*)this->pool[i];
        }

    }
    return NULL;
}


void MSProvidersPool::saveTokenFile(const QString &providerName){
    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
//        cp->saveTokenFile(this->workPath);
        cp->saveTokenFile(cp->credentialsPath);

    }
}


bool MSProvidersPool::loadTokenFile(const QString &providerName){
    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
//        return cp->loadTokenFile(this->workPath);
        return cp->loadTokenFile(cp->credentialsPath);

    }
    return false;
}


bool MSProvidersPool::refreshToken(const QString &providerName){

    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        return cp->refreshToken();

    }
    return false;
}


void MSProvidersPool::setStrategy(MSCloudProvider::SyncStrategy strategy){

    this->strategy=strategy;
}


void MSProvidersPool::setFlag(const QString &flagName, bool flagVal){

    this->flags.insert(flagName,flagVal);

}


void MSProvidersPool::setOption(const QString &optionName, const QString &optVal){

    this->options.insert(optionName,optVal);

}



