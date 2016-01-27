#include "include/msidslist.h"

MSIDsList::MSIDsList()
{
    this->ids_iterator=this->ids_list.end();
}


void MSIDsList::setList(QList<QString> list){
    this->ids_list=list;
    this->ids_iterator=this->ids_list.begin();
}

QString MSIDsList::getID(){
    if(this->ids_iterator != this->ids_list.end()){
        QString id=*this->ids_iterator;
        this->ids_iterator++;
        return id;
    }
    else{
        return "";
    }
}
