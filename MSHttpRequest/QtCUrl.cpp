/* This file is part of QtCurl
 *
 * Copyright (C) 2013-2014 Pavel Shmoylov <pavlonion@gmail.com>
 *
 * Thanks to Sergey Romanenko for QStringList options support and for
 * trace function.
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

#include "QtCUrl.h"

#include <iostream>

#include <QUrl>
#include <QDebug>
#include <QTextCodec>
#include <QStringList>


//#define QTCURL_DEBUG

CURLcode curlGlobalInit() {
    return curl_global_init(CURL_GLOBAL_ALL);
}


//int writer(char* data, size_t size, size_t nmemb, std::string* buffer) {
int writer(char* data, size_t size, size_t nmemb, void* userdata) {

    QtCUrl* curlPtr = (QtCUrl*)userdata;
    qint64 out = 0;

    if(curlPtr->outFile != nullptr){

        curlPtr->outFile->write(data, size * nmemb);
        out = size * nmemb;
    }
    else{

        //QByteArray ba(data,  size * nmemb);

        curlPtr->_buffer.append( data,  size * nmemb);
        out = size * nmemb;
    }

    return out;
}


size_t header(char* buffer, size_t size,size_t nitems, void* userdata){

    //return nitems * size;

    if(userdata == nullptr){
        return nitems * size;
    }

    QtCUrl* curlPtr = (QtCUrl*)userdata;

    QString s (QByteArray(buffer,nitems));

    if(s.indexOf(":") == -1){

        return nitems * size;
    }

    QString name = s.left(s.indexOf(":"));
    QString val = s.mid(s.indexOf(":")+1).trimmed();

    QPair<QByteArray,QByteArray> hv(name.toLocal8Bit(),val.toLocal8Bit());

    curlPtr->replyHeaders.append(hv);

    //((QtCUrl::HeaderPtr)userdata)(buffer,size,nitems,userdata);
    return nitems * size;
   // return
}


size_t reader(char* buffer, size_t size,size_t nitems, void* userdata){

    QtCUrl* curlPtr = (QtCUrl*)userdata;

    quint64 out = 0;
    quint64 realCount = 0;

    quint64 fsz = curlPtr->inpFile->size();
    quint64 pos = curlPtr->inpFile->pos();

//    qStdOut()<<"POS is "<<pos;

    if(curlPtr->payloadChunkSize == 0){ // if the file will be processed as a single piece

        if( pos + (size * nitems) > fsz){
            realCount = fsz - pos;
        }
        else{
            realCount = size * nitems;
        }

    }
    else{ // if the file splitted by chunks

        quint64 relPos = pos-curlPtr->payloadFilePosition; //position relative of chunk beginning

        if( relPos + (size * nitems) > (quint64)curlPtr->payloadChunkSize){
            realCount = curlPtr->payloadChunkSize - relPos;
        }
        else{
            realCount = size * nitems;
        }


    }

    if(realCount <= 0 ){
        return 0;
    }


    if(curlPtr->inpFile != nullptr){

//        if(curlPtr->payloadChunkSize != 0){

//            if(ppp >= curlPtr->payloadChunkSize){
//                return 0;
//            }
//        }

        out = curlPtr->inpFile->read(buffer,realCount);
//        out = size * nitems;
    }
//    else{

//        return 0/*size * nitems*/;
//    }


//    if(out == -1){
//        return 0;
//    }
    return out;

}


//void* run_easy_perform(void* obj){

//    QtCUrl* curlPtr = (QtCUrl*)obj;

//    curlPtr->exec(curlPtr->requestOptions);


//}

#ifdef QTCURL_DEBUG
int trace(CURL *handle, curl_infotype type, unsigned char *data, size_t size, void *userp)
{
    std::cerr<< data << std::endl;
    return 1;
}
#endif


QtCUrl::QtCUrl(): _textCodec(0) {
    /*
     * It's necessarily to call curl_global_init ones before the first use
     * of curl_easy_init.
     *
     * http://curl.haxx.se/libcurl/c/curl_easy_init.html
     */
    //static CURLcode __global = curlGlobalInit();
    //Q_UNUSED(__global)

    this->payloadChunkSize =0;
    this->payloadFilePosition =0;

    this->inpFile =nullptr;
    this->outFile = nullptr;
    this->headerFunction = nullptr;
    this->slist = nullptr;

    _curl = curl_easy_init();
//    _errorBuffer = new char[CURL_ERROR_SIZE];
    _errorBuffer = (char*)malloc(CURL_ERROR_SIZE);

    _replyURL = nullptr;
    //slist = nullptr;
}


QtCUrl::~QtCUrl() {

    while(_slist.count()) {
        curl_slist_free_all(_slist.first());
        _slist.removeFirst();
    }

    curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, 0L);

//    if(slist != nullptr)
//        curl_slist_free_all(slist);

    OptionsIterator i(this->requestOptions);

    while (i.hasNext()) {
        i.next();
        QVariant value = i.value();
        QVariant::Type type = value.type();


                    if (QString(value.typeName()) == "char*") {
                        delete( value.value<char*>());
        //                curl_easy_setopt(_curl, i.key(), value.value<char*>());
                    }

                    if(type == QVariant::ByteArray){
                        value.toByteArray().clear();
                    }
                    if(type == QVariant::String){
                        value.toString().clear();
                    }


        value.clear();
    }

    _buffer.clear();
    //requestOptions.clear();
    replyHeaders.clear();
    if(_replyURL != nullptr){
        curl_free(_replyURL);
    }
   //

    //curl_easy_reset(_curl);
    curl_easy_cleanup(_curl);
//    delete[] _errorBuffer;
    free(_errorBuffer);
//    curl_global_cleanup();
}


void QtCUrl::setTextCodec(const char* codecName)	{
    _textCodec = QTextCodec::codecForName(codecName);
}


void QtCUrl::setTextCodec(QTextCodec* codec) {
    _textCodec = codec;
}

QString QtCUrl::exec( Options &opt) {

    this->_buffer.clear();

    setOptions(opt);
    //
    if(this->outFile != nullptr){
        this->outFile->open(QIODevice::WriteOnly);

    }

    if(this->inpFile != nullptr){
        this->inpFile->open(QIODevice::ReadOnly);

        if(this->payloadChunkSize != 0){
            this->inpFile->seek(this->payloadFilePosition);
        }
    }


   this->_errorBuffer[0] = 0;

    _lastCode = Code(curl_easy_perform(_curl));

    curl_easy_getinfo(_curl, CURLINFO_EFFECTIVE_URL, &_replyURL);

    if(_replyURL != nullptr){
        this->replyURL = QString(_replyURL);
    }
    else{
        this->replyURL = "NO_EFFECIVE_URL";
    }





    if (_textCodec) {

        return _textCodec->toUnicode(QByteArray(_buffer.data(), _buffer.size()).constData());
    }


    curl_easy_setopt(_curl, CURLOPT_COOKIELIST, "FLUSH");


    if(this->outFile != nullptr){
        this->outFile->close();
    }

    if(this->inpFile != nullptr){
        this->inpFile->close();
    }
    //return reply;

    return ((char*)_buffer.data());;
}

QString QtCUrl::exec(){
    return this->exec(this->requestOptions);
}


void QtCUrl::setOptions(Options& opt) {
    Options defaults;
//    defaults[CURLOPT_FAILONERROR] = true;
    defaults[CURLOPT_ERRORBUFFER].setValue(_errorBuffer);

    defaults[CURLOPT_SSL_VERIFYHOST].setValue(false);
    defaults[CURLOPT_SSL_VERIFYPEER].setValue(false);

    defaults[CURLOPT_MAXREDIRS].setValue(-1);

    defaults[CURLOPT_WRITEFUNCTION].setValue(&writer);

    if(this->inpFile != nullptr){

        defaults[CURLOPT_READFUNCTION].setValue(&reader);
    }


    defaults[CURLOPT_HEADERFUNCTION].setValue(&header);

#ifdef QTCURL_DEBUG
    curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, trace);
#endif

    OptionsIterator i(defaults);

    while (i.hasNext()) {
        i.next();

        if (! opt.contains(i.key())) {
            opt[i.key()] = i.value();
        }
    }

    i = opt;

    while (i.hasNext()) {
        i.next();
        QVariant value = i.value();

        switch (value.type()) {
        case QVariant::Bool:
        case QVariant::Int: {
            int val = value.toInt();
            curl_easy_setopt(_curl, i.key(), val);
            break;
        }
        case QVariant::ByteArray: {
            QByteArray ba = value.toByteArray();
            curl_easy_setopt(_curl, i.key(), ba.constData());
            break;
        }
        case QVariant::Url: {
            QByteArray ba = value.toUrl().toEncoded();
            curl_easy_setopt(_curl, i.key(), ba.constData());
            break;
        }
        case QVariant::String: {
            std::string str = value.toString().toStdString();
            curl_easy_setopt(_curl, i.key(), str.c_str());
            break;
        }
        case QVariant::ULongLong: {
            qulonglong val = value.toULongLong();
            curl_easy_setopt(_curl, i.key(), (void*) val);
            break;
        }
        case QVariant::StringList: {
            /*struct curl_slist **/slist = NULL;
            foreach (const QString &tmp, value.toStringList()) {
                slist = curl_slist_append(slist, tmp.toUtf8().data());
            }
            _slist.append(slist);
            curl_easy_setopt(_curl, i.key(), slist);
            break;
        }
        default:
            const QString typeName = value.typeName();

            if (typeName == "QtCUrl::WriterPtr") {
                curl_easy_setopt(_curl, i.key(), value.value<WriterPtr>());
            }
            else if (typeName == "QtCUrl::HeaderPtr") {
                curl_easy_setopt(_curl, i.key(), value.value<HeaderPtr>());
            }
            else if (typeName == "QtCUrl::ReaderPtr") {
                curl_easy_setopt(_curl, i.key(), value.value<ReaderPtr>());
            }
            else if (typeName == "QtCurl*") {
                curl_easy_setopt(_curl, i.key(), value.value<QtCUrl*>());
            }
            else if (typeName == "std::string*") {
                curl_easy_setopt(_curl, i.key(), value.value<std::string*>());
            }
            else if (typeName == "char*") {
                curl_easy_setopt(_curl, i.key(), value.value<char*>());
            }
            else if (typeName == "void*") {
                curl_easy_setopt(_curl, i.key(), value.value<char*>());
            }
            else if (typeName == "QIODevice*") {
                curl_easy_setopt(_curl, i.key(), value.value<QIODevice*>());
            }
            else if (typeName == "qlonglong") {
                curl_easy_setopt(_curl, i.key(), value.value<qlonglong>());
            }
            else {
                qDebug() << "[QtCUrl] Unsupported option type: " << typeName;
            }
        }
    }
}

QString QtCUrl::escape(const QString &str){

    QByteArray s = str.toLocal8Bit();
    char* c = curl_easy_escape(_curl, s.data(),s.size() );
    QString cs(c);
    curl_free(c);
    return cs;
}







