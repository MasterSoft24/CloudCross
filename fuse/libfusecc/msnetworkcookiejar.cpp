#include "msnetworkcookiejar.h"

MSNetworkCookieJar::MSNetworkCookieJar(QObject *parent) : QObject(parent)
{

    this->cookieFile = new QTemporaryFile("cclib");
    this->cookieFile->setAutoRemove(true);
    this->cookieFile->open();

    this->name = this->cookieFile->fileName();


}

QString MSNetworkCookieJar::getFileName(){
    return this->cookieFile->fileName();
}

MSNetworkCookieJar::~MSNetworkCookieJar(){

    this->cookieFile->close();
    this->cookieFile->remove();
    delete(this->cookieFile);
    this->cookieFile = nullptr;
}

bool MSNetworkCookieJar::isCookieRemoved(){
    if(this->cookieFile == nullptr){
        return true;
    }
    else{
        return false;
    }
}
