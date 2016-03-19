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

/*
    MSOptParser - command line parameters parser class

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC

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

#ifndef MSOPTPARSER_H
#define MSOPTPARSER_H

#include <QObject>
#include <QList>
#include <QStringList>



typedef struct _optItem{

    int num; // must be a 1 and so on;
    QString shortOpt;
    QString longOpt;
    int paramCount=0;// must by 0 or 1
    //QString defaultParam;

}optItem;


class MSOptParser : public QObject
{
    Q_OBJECT

private:

    QStringList  input;
    QStringList::iterator iit;
    QString getShort(QStringList list);
    QString getLong(QStringList list);
    int getParamCount(QStringList list);

public:
    explicit MSOptParser(QObject *parent = 0);

    QList<optItem> opts;
    QString optarg;
    int erorrNum=0;
    QString errorString;

    void insertOption(int num, QString optString);
    void parse(QStringList list);
    int get(); // return 0 if no matches found, -1 if error and value of num member if arg found

    bool getArg();

    QString getParamByName(QString paramName);// get named parameters value (if this parameter is exists) from input parameters list

signals:

public slots:
};

#endif // MSOPTPARSER_H
