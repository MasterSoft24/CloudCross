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

#ifndef MSREQUEST_H
#define MSREQUEST_H

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QEventLoop>
#include "qstdout.h"
#include <QFile>
#include <QDataStream>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QJsonObject>
#include <QNetworkProxy>

class MSRequest : public QObject , QNetworkRequest
{
  Q_OBJECT

public:
    MSRequest(QNetworkProxy* proxy=0);
    ~MSRequest();

//private:

    QUrl* url;
    QUrlQuery* query;
    QNetworkAccessManager* manager;

    QString requestMethod; // get, post, put, post-multipart etc
    QEventLoop* loop;

    QFile* outFile;
    QDataStream* outFileStream;

public:

    QNetworkReply* lastReply;// deprecated
    QNetworkReply* currentReply;// deprecated

    QByteArray replyText;
    QVariant replyAttribute;
    QString replyURL;
    QNetworkReply::NetworkError replyError;
    QString replyErrorText;

    bool notUseContentType=false; // do not use any Content-Type header for request (for example needed for some Dropbox requests)

    QList<QPair<QByteArray,QByteArray>> replyHeaders;

    QByteArray readReplyText();


    bool setMethod(QString method);
    void setRequestUrl(QString url);
    void addQueryItem(QString itemName, QString itemValue );

    void addHeader(QString headerName, QString headerValue);
    void addHeader(QByteArray headerName, QByteArray headerValue);

    void exec();
    void post(QByteArray data);
    void put(QByteArray data);
    void put(QIODevice* data);
    void methodCharger(QNetworkRequest req);
    void methodCharger(QNetworkRequest req,QString path);

    void download(QString url);
    void download(QString url,QString path);
    void deleteResource();

    bool replyOK();
    void printReplyError();


    // proxy block

    void setProxy(QNetworkProxy *proxy);

    // cookie functions block

    QNetworkCookieJar* cookieJar;

    void MSsetCookieJar(QNetworkCookieJar *cookie);
    QJsonObject cookieToJSON();
    bool cookieFromJSON(QJsonObject cookie);

private slots:

    void requestFinished(QNetworkReply *reply);

    void doDownloadProgress(qint64 avail,qint64 total);

    void doReadyRead();

};


#endif // MSREQUEST_H


