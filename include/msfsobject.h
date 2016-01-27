#ifndef MSFSOBJECT_H
#define MSFSOBJECT_H

#include "msremotefsobject.h"
#include "mslocalfsobject.h"

// incapsulate remote and local fs objects
class MSFSObject
{
public:
    MSFSObject();

    enum ObjectState{
                ChangedRemote=1,
                ChangedLocal=2,
                NewRemote=3,
                NewLocal=4,
                DeleteRemote=5,
                DeleteLocal=6,
                Sync=0
    };



    MSLocalFSObject local;
    MSRemoteFSObject remote;

    ObjectState state;

    QString path;
    QString fileName;

    void getLocalMimeType(QString path);

};

#endif // MSFSOBJECT_H
