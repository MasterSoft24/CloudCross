#ifndef CCFDCOMMAND_H
#define CCFDCOMMAND_H

#include <QObject>
#include <QStringList>
#include<QtNetwork/QLocalSocket>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>


#include "include/mscloudprovider.h"
#include "include/msproviderspool.h"

enum ProviderType{

    Google,
    Dropbox,
    Yandex,
    Mailru,
    OneDrive
};

class CCFDCommand : public QObject
{
    Q_OBJECT
public:
    explicit CCFDCommand(QObject *parent = 0);


    void parseParameters();

    QJsonObject params;
    QLocalSocket* socket;
    QString command;
    QString socket_name;
    QString path;
    ProviderType provider;


signals:
    void finished();
    void result(const QString& r);

public slots:

    void run();
};

#endif // CCFDCOMMAND_H
