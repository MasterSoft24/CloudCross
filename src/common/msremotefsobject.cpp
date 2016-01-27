#include "include/msremotefsobject.h"

MSRemoteFSObject::MSRemoteFSObject()
{
    this->fileSize=0;
    this->objectType=MSRemoteFSObject::none;
    this->md5Hash="";
    this->exist=false;
}
