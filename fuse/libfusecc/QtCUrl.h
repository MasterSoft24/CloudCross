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

#ifndef QTCURL_H
#define QTCURL_H

#include <curl/curl.h>
#include <QHash>
#include <QVariant>
#include <QLinkedList>
#include <QByteArray>
#include <QIODevice>
#include <QList>
#include <QPair>
class QTextCodec;


//Q_DECLARE_METATYPE(QByteArray*)

class QtCUrl {
public:
    typedef QHash<CURLoption, QVariant> Options;
    typedef QHashIterator<CURLoption, QVariant> OptionsIterator;
    typedef int (*WriterPtr)(char*, size_t, size_t, std::string*);

    // Changes for a CloudCross MSRequest class

    typedef size_t (*ReaderPtr)(void*, size_t , size_t , void*);
    typedef size_t (*HeaderPtr)(char*, size_t ,size_t, void*);


    Options requestOptions;

    QList<QPair<QByteArray,QByteArray>> replyHeaders;

    QString escape(QString str);
    //    QByteArray escape(QByteArray str);

     HeaderPtr headerFunction;

    class Code {
    public:
        Code(CURLcode code = CURLE_OK): _code(code) {}
        QString text() { return curl_easy_strerror(_code); }
        inline CURLcode code() { return _code; }
        inline bool isOk() { return _code == CURLE_OK; }





    private:
        CURLcode _code;
    };

    QtCUrl();
    virtual ~QtCUrl();

    QString exec(Options& opt);
    QString exec();

    QByteArray buffer() const {
        return QByteArray(_buffer.data(), _buffer.size());
    }
    inline Code lastError() { return _lastCode; }
    QString errorBuffer() { return _errorBuffer; }
    void setTextCodec(const char* codecName);
    void setTextCodec(QTextCodec* codec);



protected:
    void setOptions(Options& opt);

private:
    CURL* _curl;
    char* _errorBuffer;
    std::string _buffer;

    QByteArray replyBuffer;

    Code _lastCode;
    QTextCodec* _textCodec;
    QLinkedList<curl_slist*> _slist;




};

Q_DECLARE_METATYPE(QtCUrl::WriterPtr)
Q_DECLARE_METATYPE(QtCUrl::ReaderPtr)
Q_DECLARE_METATYPE(QtCUrl::HeaderPtr)
Q_DECLARE_METATYPE(std::string*)
Q_DECLARE_METATYPE(char*)
Q_DECLARE_METATYPE(void*)
//Q_DECLARE_METATYPE(QIODevice*)
Q_DECLARE_METATYPE(QtCUrl*)

#endif // QTCURL_H
