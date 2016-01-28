/*
    CloudCross: Opensource program for syncronization of local files and folders with Google Drive

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation version 2
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
