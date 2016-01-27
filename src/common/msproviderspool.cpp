#include "include/msproviderspool.h"
#include <QDir>


MSProvidersPool::MSProvidersPool()
{
    this->getCurrentPath();
    this->strategy=MSCloudProvider::SyncStrategy::PreferLocal;

}


void MSProvidersPool::addProvider(MSCloudProvider *provider){

    MSCloudProvider* cp=this->getProvider(provider->providerName);
    if(cp==NULL){
        provider->currentPath=this->currentPath;
        provider->workPath=this->workPath;
        provider->strategy=this->strategy;

        QHash<QString,bool>::iterator it=this->flags.begin();
        while(it != this->flags.end()){

            provider->setFlag(it.key(),it.value());
            it++;
        }

        provider->loadStateFile();

        this->pool.append(provider);

        return;
    }
}

void MSProvidersPool::getCurrentPath(){
    this->currentPath=QDir::currentPath();
    this->workPath=this->currentPath;
}


void MSProvidersPool::setWorkPath(QString path){
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


void MSProvidersPool::loadTokenFile(QString providerName){
    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        cp->loadTokenFile(this->workPath);

    }

}


void MSProvidersPool::refreshToken(QString providerName){

    MSCloudProvider* cp=this->getProvider(providerName);
    if(cp!=NULL){
        cp->refreshToken();

    }
}


void MSProvidersPool::setStrategy(MSCloudProvider::SyncStrategy strategy){

    this->strategy=strategy;
}


void MSProvidersPool::setFlag(QString flagName, bool flagVal){

    this->flags.insert(flagName,flagVal);

}

