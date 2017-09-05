




#ifndef MSNETWORKCOOKIEJAR_H
#define MSNETWORKCOOKIEJAR_H

#include <QObject>
#include <QTemporaryFile>

class MSNetworkCookieJar : public QObject
{
    Q_OBJECT

private:

    QTemporaryFile* cookieFile;

public:

    QString name;

    explicit MSNetworkCookieJar(QObject *parent = 0);
    QString getFileName();
    ~MSNetworkCookieJar();

    bool isCookieRemoved();

signals:

public slots:
};

#endif // MSNETWORKCOOKIEJAR_H
