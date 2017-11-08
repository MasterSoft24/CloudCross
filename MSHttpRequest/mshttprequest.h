#ifndef MSHTTPREQUEST_H
#define MSHTTPREQUEST_H

#include <QObject>
#include <QNetworkReply>
#include <QFile>
#include <QDataStream>
#include <QProcess>

#include "QtCUrl.h"
#include "msnetworkcookiejar.h"
#include "qmultibuffer.h"
#include "msnetworkproxy.h"

class MSHttpRequest : public QObject
{
    Q_OBJECT
public:

    enum dataStreamTypes{
        DS_None,
        DS_File,
        DS_ByteArray,
        DS_MultiBuffer
    };


     MSHttpRequest(MSNetworkProxy* proxy);
    ~MSHttpRequest();

    QString requestMethod; // get, post, put, post-multipart etc
    QString requestURL;

    MSNetworkProxy* proxy;


    QHash<QString,QString> queryItems;
    QHash<QString,QString> requestHeaders;

    QtCUrl *cUrlObject;

    QString replyText;
    QNetworkReply::NetworkError replyError;
    QString replyErrorText;
    MSNetworkCookieJar* cookieJarObject;

    // for serialize
    QByteArray dataStreamByteArray;
    QString dataStreamFile;
    QList<QPair<dataStreamTypes,QByteArray>> dataStreamMultiBuffer;
    dataStreamTypes dataStreamType;


    // methods

    void setProxy(MSNetworkProxy* proxy);
    void disableProxy();

    bool setMethod(const QString &method);
    void setPayloadChunkData(qint64 size,quint64 pos);
    void setRequestUrl(const QString &url);
    void addQueryItem(const QString &itemName, const QString &itemValue );

    void addHeader(const QString &headerName, const QString &headerValue);
    void addHeader(const QByteArray &headerName, const QByteArray &headerValue);
    QString getReplyHeader(const QByteArray &headerName);

    void setOutputFile(const QString &fileName);
    //
    void setInputDataStream(const QByteArray &data);
    void setInputDataStream(QFile* data);
    void setInputDataStream(QMultiBuffer* data);

    QString replyURL();

    void MSsetCookieJar(MSNetworkCookieJar *cookie);
    MSNetworkCookieJar *getCookieJar();


    // request executors

    void post(const QByteArray &data);
    void post(QIODevice *data);
    void put(const QByteArray &data);
    void put(QIODevice* data);
    void deleteResource();
    void get();

    void exec(); // run with executor process

    void raw_exec(const QString &reqestURL);

    void download(const QString &url);
    void download(const QString &url, const QString &path);


    QString readReplyText();
    bool replyOK();
    void printReplyError();
signals:

public slots:
    void readExecutorOutput();
};

#endif // MSHTTPREQUEST_H
