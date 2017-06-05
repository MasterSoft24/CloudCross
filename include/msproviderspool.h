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

#ifndef MSPROVIDERSPOOL_H
#define MSPROVIDERSPOOL_H

#include <QObject>
#include <QDateTime>
#include "qstdout.h"
#include "src/GoogleDrive/msgoogledrive.h"
#include "src/Dropbox/msdropbox.h"
#include "src/YandexDisk/msyandexdisk.h"
#include "src/MailRu/msmailru.h"

#include "src/OneDrive/msonedrive.h"



class MSProvidersPool
{

private:




public:
    MSProvidersPool();

    QString generateRandom(int count);

    QList<MSCloudProvider*> pool;

    MSCloudProvider* getProvider(const QString &providerName)    ;

    QString currentPath;// directory where program was run
    QString workPath;// path set with -p option

    MSCloudProvider::SyncStrategy strategy; // sync strategy

    QHash<QString,bool>flags;
    QHash<QString,QString>options;

    QString proxyAddrString;
    QString proxyTypeString;

    void addProvider(MSCloudProvider* provider, bool statelessMode=false);

    void getCurrentPath();
    void setWorkPath(const QString &path);

    void saveTokenFile(const QString &providerName);

    bool loadTokenFile(const QString &providerName);

    bool refreshToken(const QString &providerName);

    void setStrategy(MSCloudProvider::SyncStrategy strategy);

    void setFlag(const QString &flagName,bool flagVal);

    void setOption(const QString &optionName, const QString &optVal);

};

#endif // MSPROVIDERSPOOL_H
