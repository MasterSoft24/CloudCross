#ifndef MSREMOTEFSOBJECT_H
#define MSREMOTEFSOBJECT_H
#include <QTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

class MSRemoteFSObject
{
public:
    MSRemoteFSObject();

    enum Type{
               none=0,
               file=1,
               folder=2
             };

    bool exist;
    Type objectType;

    qint64 modifiedDate;

    QString md5Hash;

    int fileSize;

    QJsonObject data;

};

#endif // MSREMOTEFSOBJECT_H
