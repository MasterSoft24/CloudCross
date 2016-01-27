#ifndef MSREQUEST_H
#define MSREQUEST_H

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

class MSRequest : public QObject , QNetworkRequest
{
  Q_OBJECT

public:
    MSRequest();

private:

    QUrl* url;
    QUrlQuery* query;
    QNetworkAccessManager* manager;

    QString requestMethod; // get, post, put, post-multipart etc

public:

    QNetworkReply* lastReply;


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


