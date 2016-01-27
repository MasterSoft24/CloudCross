#include "include/msfsobject.h"

MSFSObject::MSFSObject()
{

}


void MSFSObject::getLocalMimeType(QString path){

        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(path+this->path+this->fileName);
        this->local.mimeType=type.name();
}
