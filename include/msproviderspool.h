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

#ifndef MSPROVIDERSPOOL_H
#define MSPROVIDERSPOOL_H

#include <QObject>
#include "qstdout.h"
#include "src/GoogleDrive/msgoogledrive.h"
#include "src/Dropbox/msdropbox.h"


class MSProvidersPool
{

private:




public:
    MSProvidersPool();

    QString generateRandom(int count);

    QList<MSCloudProvider*> pool;

    MSCloudProvider* getProvider(QString providerName)    ;

    QString currentPath;// directory where program was run
    QString workPath;// path set with -p option

    MSCloudProvider::SyncStrategy strategy; // sync strategy

    QHash<QString,bool>flags;
    QHash<QString,QString>options;



    void addProvider(MSCloudProvider* provider, bool statelessMode=false);

    void getCurrentPath();
    void setWorkPath(QString path);

    void saveTokenFile(QString providerName);

    bool loadTokenFile(QString providerName);

    bool refreshToken(QString providerName);

    void setStrategy(MSCloudProvider::SyncStrategy strategy);

    void setFlag(QString flagName,bool flagVal);

    void setOption(QString optionName,QString optVal);

};

#endif // MSPROVIDERSPOOL_H
