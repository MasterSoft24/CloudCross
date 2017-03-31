#ifndef CCFW_H
#define CCFW_H

#include <sys/types.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <map>

using namespace std;
using json = nlohmann::json;


class MSLocalFSObject
{
public:
    MSLocalFSObject(){
        this->fileSize=0;
        this->objectType=MSLocalFSObject::none;
        this->md5Hash="";
        this->mimeType="";
        this->exist=false;
    }

    enum Type{
               none=0,
               file=1,
               folder=2
             };

    bool exist;

    Type objectType;

    long long int modifiedDate;

    string md5Hash;

    int fileSize;

    string mimeType;

    string newRemoteID;// for files and folder who's will be uploaded. Set after upload procedure from server response




};



//////////////////////////////////////////////////

class MSRemoteFSObject
{
public:
    MSRemoteFSObject(){
        this->fileSize=0;
        this->objectType=MSRemoteFSObject::none;
        this->md5Hash="";
        this->exist=false;
    }

    enum Type{
               none=0,
               file=1,
               folder=2
             };

    bool exist;
    Type objectType;

    long long int modifiedDate;

    string md5Hash;

    int fileSize;

    json data;

};



///////////////////////////////////////////////////


class MSFSObject
{
public:
    MSFSObject(){

    }

    enum ObjectState{
                ChangedRemote=1,
                ChangedLocal=2,
                NewRemote=3,
                NewLocal=4,
                DeleteRemote=5,
                DeleteLocal=6,
                Sync=0,
                ErrorState=100
    };



    MSLocalFSObject local;
    MSRemoteFSObject remote;

    ObjectState state;

    string path;
    string fileName;
    bool isDocFormat;


    // for workers use only

    int refCount;   // -1= a file don't opened and don't cached (or closed and wiped)
                    //  0= a file don't opened but cached
                    // >0= a file cached and opened



//    void getLocalMimeType(QString path);

};

///////////////////////////////////////////////////







#endif // CCFW_H
