/* This file is part of QMultiBuffer
 *
 * Copyright (C) 2017 Vladimir Kamensky <mastersoft24@yandex.ru>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software Foundation.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "qmultibuffer.h"

QMultiBuffer::QMultiBuffer()
{

    this->cursor =0;
}

QMultiBuffer::~QMultiBuffer()
{
    foreach (slotBound b, this->items) {
        if(QString(b.slot.typeName()) == "QBuffer*"){
            (((QIODevice*)qvariant_cast<QIODevice*>(b.slot)))->deleteLater();
        }
    }
}

void QMultiBuffer::append(QIODevice* d){

    slotBound s = {this->items.size(),0,0,""};

    QFile* f = (QFile*)d;
    if((f!= nullptr)&&(f->fileName()!="")){
        s.fileName = f->fileName();
    }

    if(this->items.size() == 0){
        s.beginPos = 0;
        s.endPos = d->size()-1;
    }
    else{

        s.beginPos = this->items[this->items.size()-1].endPos+1;
        s.endPos = s.beginPos + d->size()-1;
    }

    s.slot = qVariantFromValue(d);
    this->items.append(s);
}



void QMultiBuffer::append(QByteArray *d){


    slotBound s = {this->items.size(),0,0,""};

    if(this->items.size() == 0){
        s.beginPos = 0;
        s.endPos = d->size()-1;
    }
    else{

        s.beginPos = this->items[this->items.size()-1].endPos+1;
        s.endPos = s.beginPos + d->size()-1;
    }

    s.slot = qVariantFromValue(new QBuffer(d));
    this->items.append(s);

}



qint64 QMultiBuffer::getSlotByPosition(qint64 pos){

    for(int i=0; i< this->items.size();i++){

        if( (pos >= this->items[i].beginPos) && (pos <= this->items[i].endPos)){
            return i;
        }
    }
    return -1;
}




qint64 QMultiBuffer::readData(char *data, qint64 maxlen){

     qint64 currentSlot = this->getSlotByPosition(this->cursor);
     if(currentSlot == -1){
         return -1;
     }

     qint64 writed = 0;

     while (maxlen > 0) {

         QIODevice* bf = qvariant_cast<QIODevice*> (this->items[currentSlot].slot);

         if(!bf->isOpen()){
             bf->open(QIODevice::ReadOnly);
         }

         bf->seek(this->cursor - this->items[currentSlot].beginPos);

         qint64 w = bf->read(data + writed, maxlen);

         if( w < 0){ // error
             ((QIODevice*)(qvariant_cast<QIODevice*> (this->items[currentSlot].slot)))->close();
             break;
         }

         if( w == 0){ // possible end of slot reached

             if(currentSlot == this->items.size()-1){// last slot
                 ((QIODevice*)(qvariant_cast<QIODevice*> (this->items[currentSlot].slot)))->close();
                 break;
             }
             else{
                ((QIODevice*)(qvariant_cast<QIODevice*> (this->items[currentSlot].slot)))->close();
                currentSlot++;
             }
         }

         this->cursor += w;


         writed +=w;
         maxlen -= w;
     }

     return writed;
}

qint64 QMultiBuffer::writeData(const char *data, qint64 len){

    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}



bool QMultiBuffer::seek(qint64 pos){

    qint64 sz = this->size();

    if(pos <= sz){
        this->cursor = pos;
        return true;
    }
    else{
        return false;
    }


}



qint64 QMultiBuffer::size() const{

    qint64 sz=0;

    for(int i=0; i< this->items.size();i++){

        QString vt = this->items[i].slot.typeName();

        if( vt == "QBuffer*"){
            sz += ((QBuffer*)(qvariant_cast<QBuffer*> (this->items[i].slot)))->size();
        }
        if( vt == "QIODevice*"){
            sz += ((QFile*)(qvariant_cast<QFile*> (this->items[i].slot)))->size();
        }
    }

    return sz;
}

bool QMultiBuffer::open(QIODevice::OpenMode mode){

    this->cursor = 0;
    return QIODevice::open(mode);

}
