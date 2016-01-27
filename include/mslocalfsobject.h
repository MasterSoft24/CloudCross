#ifndef MSLOCALFSOBJECT_H
#define MSLOCALFSOBJECT_H
#include <QString>
#include <QMimeDatabase>
#include <QMimeType>

class MSLocalFSObject
{
public:
    MSLocalFSObject();

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

    QString mimeType;

    QString newRemoteID;// for files and folder who's will be uploaded. Set after upload procedure from server response




};

#endif // MSLOCALFSOBJECT_H
