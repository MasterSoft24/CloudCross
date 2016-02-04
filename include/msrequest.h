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

#ifndef MSREQUEST_H
#define MSREQUEST_H

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QEventLoop>

class MSRequest : public QObject , QNetworkRequest
{
  Q_OBJECT

public:
    MSRequest();
    ~MSRequest();

private:

    QUrl* url;
    QUrlQuery* query;
    QNetworkAccessManager* manager;

    QString requestMethod; // get, post, put, post-multipart etc
    QEventLoop* loop;


public:

    QNetworkReply* lastReply;// deprecated

    QByteArray replyText;
    QVariant replyAttribute;

    QByteArray readReplyText();


    bool setMethod(QString method);
    void setRequestUrl(QString url);
    void addQueryItem(QString itemName, QString itemValue );

    void addHeader(QString headerName, QString headerValue);
    void addHeader(QByteArray headerName, QByteArray headerValue);

    void exec();
    void post(QByteArray data);
    void put(QByteArray data);
    void methodCharger(QNetworkRequest req);


private slots:

    void requestFinished(QNetworkReply *reply);

};


#endif // MSREQUEST_H


