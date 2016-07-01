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

#include "include/msproviderspool.h"
#include <QDir>


MSProvidersPool::MSProvidersPool()
{
    this->getCurrentPath();
    this->strategy=MSCloudProvider::SyncStrategy::PreferLocal;

}


QString MSProvidersPool::generateRandom(int count){

    int Low=35;
    int High=127;
    qsrand(qrand());

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
}


void MSProvidersPool::setWorkPath(QString path){

#ifdef Q_OS_WIN
   path=path.replace("\\","/") ;
#endif
    this->workPath=path;

}

MSCloudProvider* MSProvidersPool::getProvider(QString providerName){
    for(int i=0; i< this->pool.size();i++){

        if(this->pool[i]->providerName==providerName){
            return (MSCloudProvider*)this->pool[i];
        }

    }
    return NULL;
}


void MSProvidersPool::saveTokenFile(QString providerName){
    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        cp->saveTokenFile(this->workPath);

    }
}


bool MSProvidersPool::loadTokenFile(QString providerName){
    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        return cp->loadTokenFile(this->workPath);

    }
    return false;
}


bool MSProvidersPool::refreshToken(QString providerName){

    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        return cp->refreshToken();

    }
    return false;
}


void MSProvidersPool::setStrategy(MSCloudProvider::SyncStrategy strategy){

    this->strategy=strategy;
}


void MSProvidersPool::setFlag(QString flagName, bool flagVal){

    this->flags.insert(flagName,flagVal);

}


void MSProvidersPool::setOption(QString optionName, QString optVal){

    this->options.insert(optionName,optVal);

}



