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

#ifndef QMULTIBUFFER_H
#define QMULTIBUFFER_H
#include <QIODevice>
#include <QVariant>
#include <QBuffer>
#include <QFile>
#include <QByteArray>
#include <QDebug>

Q_DECLARE_METATYPE(QBuffer*)
Q_DECLARE_METATYPE(QIODevice*)



class QMultiBuffer : public QIODevice
{
public:

    typedef struct slotBound{

        QVariant slot;
        qint64 beginPos;
        qint64 endPos;
        QString fileName; // used if this slot present a file and empty otherwise

    }_slotBound;


    QMultiBuffer();
    ~QMultiBuffer();

    void append(QIODevice *d);
    void append(QByteArray *d);




    QList<slotBound> items;
private:    qint64 cursor;

    qint64 getSlotByPosition(qint64 pos);


    // QIODevice interface
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    // QIODevice interface
public:
    bool seek(qint64 pos);
    qint64 size() const;
    bool open(OpenMode mode);




};





#endif // QMULTIBUFFER_H
