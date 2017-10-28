




#ifndef MSNETWORKCOOKIEJAR_H
#define MSNETWORKCOOKIEJAR_H

#include <QObject>
#include <QTemporaryFile>
#include <QDebug>
#include <QDir>

class MSNetworkCookieJar : public QObject
{
    Q_OBJECT

private:



public:

    QString name;
    QTemporaryFile* cookieFile;

    explicit MSNetworkCookieJar(QObject *parent = 0);

    QString getFileName();
    ~MSNetworkCookieJar();

    bool isCookieRemoved();

    MSNetworkCookieJar& operator =(const MSNetworkCookieJar &c){

        this->cookieFile = c.cookieFile;
        this->name = c.name;

        return *this;
    }

signals:

public slots:
};

#endif // MSNETWORKCOOKIEJAR_H
