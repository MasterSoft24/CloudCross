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

#ifndef MSFSOBJECT_H
#define MSFSOBJECT_H

#include "msremotefsobject.h"
#include "mslocalfsobject.h"

// incapsulate remote and local fs objects
class MSFSObject
{
public:
    MSFSObject();

    enum ObjectState{
                ChangedRemote=1,
                ChangedLocal=2,
                NewRemote=3,
                NewLocal=4,
                DeleteRemote=5,
                DeleteLocal=6,
                Sync=0
    };



    MSLocalFSObject local;
    MSRemoteFSObject remote;

    ObjectState state;

    QString path;
    QString fileName;
    bool isDocFormat;

    void getLocalMimeType(QString path);

};

#endif // MSFSOBJECT_H
