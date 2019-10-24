/*

    This file is a part of CloudCross FUSE Project

    Copyright (C) 2017  Vladimir Kamensky
    Copyright (C) 2017  Master Soft LLC.
    All rights reserved.

*/

/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2017  Vladimir Kamensky
    Copyright (C) 2017  Master Soft LLC.
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


#include "msnetworkcookiejar.h"

MSNetworkCookieJar::MSNetworkCookieJar(QObject *parent) : QObject(parent)
{

    this->cookieFile = new QTemporaryFile(QDir::tempPath()+"/cclib");
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
    return this->cookieFile == nullptr;
}

//MSNetworkCookieJar MSNetworkCookieJar::operator =(MSNetworkCookieJar c)
