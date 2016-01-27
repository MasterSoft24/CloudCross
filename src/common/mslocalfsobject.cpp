#include "include/mslocalfsobject.h"

MSLocalFSObject::MSLocalFSObject()
{
    this->fileSize=0;
    this->objectType=MSLocalFSObject::none;
    this->md5Hash="";
    this->mimeType="";
    this->exist=false;
}



