/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.
    All rights reserved.


  BSD License

  Redistribution and use in source and binary forms, with or without modification, are
  permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice, this list
    of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.
  - Neither the name of the "Vladimir Kamensky" or "Master Soft LLC." nor the names of
    its contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY E
  XPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES O
  F MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SH
  ALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENT
  AL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROC
  UREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS I
  NTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRI
  CT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF T
  HE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtGlobal>

#include <QCoreApplication>

//#include "include/msrequest.h"
#include "include/msoptparser.h"
#include "include/msproviderspool.h"
#include <QNetworkReply>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDateTime>
#include <QTextCodec>

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QUuid>


#include <QSysInfo>
#include <sys/utsname.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>




#define APP_MAJOR_VERSION 1
#define APP_MINOR_VERSION 4
#define APP_BUILD_NUMBER  "1-rc2"

#define CCROSS_HOME_DIR ".ccross"
#define CCROSS_CONFIG_FILE "ccross.conf"

#ifdef Q_OS_WIN
    #define APP_SUFFIX " for Windows"
#else

    #define APP_SUFFIX " for Linux"
#endif

#define APP_NAME "CloudCross"


#define USG "ccfd-WhfNxPk-544h-gaQmZog39q" // for use in fuse





void printVersion(){
    qStdOut() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX <<endl;
}

void printHelp(){
    printVersion();
    qStdOut()<< QObject::tr("is a opensource program for sync local files with a many cloud storages.\n") <<endl;
    qStdOut()<< QObject::tr("Options:") <<endl;
    qStdOut()<< QObject::tr("   -h [ --help ]              Produce help message") <<endl;
    qStdOut()<< QObject::tr("   -v [ --version ]           Display CloudCross version") <<endl;
    qStdOut()<< QObject::tr("   -a [ --auth ]              Request authorization token") <<endl;
    qStdOut()<< QObject::tr("   -p [ --path ] arg          Path to sync directory") <<endl;
    qStdOut()<< QObject::tr("   --dry-run                  Only detect which files need to be uploaded/downloaded,\n"
                            "                              without actually performing them.") <<endl;
    qStdOut()<< QObject::tr("   -s [ --list ]              Print remote cloud file list") <<endl;
    qStdOut()<< QObject::tr("   --use-include              Use .include file. Without this option by default use .exclude file.\n"
                            "                              If these files does'nt exists, they  are ignore") <<endl;
    qStdOut()<< QObject::tr("   --prefer arg               Define sync strategy. It can be a one of \"remote\" or \"local\". By default it's \"local\"") <<endl;
    qStdOut()<< QObject::tr("   --no-hidden                Not sync hidden files and folders") <<endl;
    qStdOut()<< QObject::tr("   --no-new-rev               Do not create new revisions of files, overwrite their instead") <<endl;
    qStdOut()<< QObject::tr("   --convert-doc              Convert office document to Google Doc format when upload\n"
                            "                              and convert him back when download") <<endl;
    qStdOut()<< QObject::tr("   --force arg                Forcing upload or download files. It can be a one of \"upload\" or \"download\".\n"
                            "                              This option overrides --prefer option value.") <<endl;

    qStdOut()<< QObject::tr("   --provider arg             Set cloud provider for current sync operation. On this moment this option can be \n"
                            "                              a \"google\", \"yandex\", \"mailru\", \"onedrive\" or \"dropbox\". Default provider is Google Drive") <<endl;

//    qStdOut()<< QObject::tr("   --direct-upload url path   Allow upload file directly to cloud from URL.\n"
//                            "                              All options, except --provider and --path, are ignored.\n"
//                            "                              Uploaded file will be stored on remote storage into location which was defined by path.\n"
//                            "                              NOTE: Direct upload to OneDrive does not supported.") <<endl;
    qStdOut()<< QObject::tr("   --login arg                Set login for access to cloud provider. \n"
                            "                              Now it used only for Cloud Mail.ru") <<endl;
    qStdOut()<< QObject::tr("   --password arg             Set password for access to cloud provider. \n"
                            "                              Now it used only for Cloud Mail.ru") <<endl;
    qStdOut()<< QObject::tr("   --http-proxy arg           Use http proxy server for connection to cloud provider. \n"
                            "                              <arg> must be in a ip_address_or_host_name:port_number format") <<endl;
    qStdOut()<< QObject::tr("   --socks5-proxy arg         Use socks5 proxy server for connection to cloud provider. \n"
                            "                              <arg> must be in a ip_address_or_host_name:port_number format") <<endl;
    qStdOut()<< QObject::tr("   --cloud-space              Showing total and free space  of cloud \n")<<endl;
    qStdOut()<< QObject::tr("   --filter-type              Filter type for .include and .exclude files. Can be set to \"regexp\" or \"wildcard\". "
                            "                              Ignored if it set in files") <<endl;
    qStdOut()<< QObject::tr("   --single-thread            Run as single threaded") <<endl;

    qStdOut()<< QObject::tr("   --low-memory               Reduce memory utilization during reading a remote file list. Using of this"
                          "                                option may do increase of synchronization time ") <<endl;

    qStdOut()<< QObject::tr("   --empty-trash              Delete all files from cloud trash bin.")<<endl;

    qStdOut()<< QObject::tr("   --no-sync                  If this option is set synchronization mechanism will be disabled and remote file list"
                            "                              not be a readed. Local files will be uploaded without consideration of existence"
                            "                              of this files on remote. Use with carefully")<<endl;
}



void authGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    gdp->auth();
    if(gdp->providerAuthStatus){

        providers->addProvider(gdp,true);
        providers->saveTokenFile("GoogleDrive");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }
}


void listGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(gdp,true);
    if(!providers->loadTokenFile("GoogleDrive")){
        return ;
    }

    if(! providers->refreshToken("GoogleDrive")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    QHash<QString,MSFSObject> list=gdp->getRemoteFileList();

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=list.begin();

    while(li != list.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}


void syncGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(gdp);
    if(! providers->loadTokenFile("GoogleDrive")){
        exit(0);
    }

    if(!providers->refreshToken("GoogleDrive")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    gdp->createSyncFileList();

}


void infoGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(gdp,true);
    if(!providers->loadTokenFile("GoogleDrive")){
        return ;
    }

    if(! providers->refreshToken("GoogleDrive")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }


    QString info=gdp->getInfo();

    if(info == "false"){

        qStdOut()<< "Error getting cloud information " <<endl;
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject job = json.object();

    double usage=  job["usage"].toString().toDouble();
    double total=  job["total"].toString().toDouble();
    double free= total - usage;

    qStdOut()<< job["account"].toString()<< endl << endl << "total: "<< (uint64_t)total <<endl << "usage: "<< (uint64_t)usage << endl << "free: "<< (uint64_t)free <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1048576<<" MB" <<endl << "usage: "<< (uint64_t)usage/1048576<<" MB"  << endl << "free: "<< (uint64_t)free/1048576<<" MB"  <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1073741824<<" GB" <<endl << "usage: "<< (uint64_t)usage/1073741824<<" GB"  << endl << "free: "<< (uint64_t)free/1073741824<<" GB"  <<endl;


}


void emptyTrashGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(gdp);
    if(! providers->loadTokenFile("GoogleDrive")){
        exit(0);
    }

    if(!providers->refreshToken("GoogleDrive")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    gdp->remote_file_empty_trash();

}


void authDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    dbp->auth();
    if(dbp->providerAuthStatus){

        providers->addProvider(dbp,true);
        providers->saveTokenFile("Dropbox");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }
}


void listDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp,true);
    if(!providers->loadTokenFile("Dropbox")){
        return ;
    }

    if(! providers->refreshToken("Dropbox")){

        qStdOut()<< "Unauthorized client"<<endl;
        exit(0);
    }

    dbp->readRemote();



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=dbp->syncFileList.begin();

    while(li != dbp->syncFileList.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}


void syncDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp);
    if(! providers->loadTokenFile("Dropbox")){
        exit(0);
    }

    if(!providers->refreshToken("Dropbox")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    dbp->createSyncFileList();

}


void infoDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp,true);
    if(!providers->loadTokenFile("Dropbox")){
        return ;
    }

    if(! providers->refreshToken("Dropbox")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }


    QString info=dbp->getInfo();

    if(info == "false"){

        qStdOut()<< "Error getting cloud information " <<endl;
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject job = json.object();

    double usage=  job["usage"].toString().toDouble();
    double total=  job["total"].toString().toDouble();
    double free= total - usage;

    qStdOut()<< job["account"].toString()<< endl << endl << "total: "<< (uint64_t)total <<endl << "usage: "<< (uint64_t)usage << endl << "free: "<< (uint64_t)free <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1048576<<" MB" <<endl << "usage: "<< (uint64_t)usage/1048576<<" MB"  << endl << "free: "<< (uint64_t)free/1048576<<" MB"  <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1073741824<<" GB" <<endl << "usage: "<< (uint64_t)usage/1073741824<<" GB"  << endl << "free: "<< (uint64_t)free/1073741824<<" GB"  <<endl;


}

void emptyTrashDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp);
    if(! providers->loadTokenFile("Dropbox")){
        exit(0);
    }

    if(!providers->refreshToken("Dropbox")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    dbp->remote_file_empty_trash();
}


void authYandex(MSProvidersPool* providers){

    MSYandexDisk* ydp=new MSYandexDisk();

    ydp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    ydp->auth();
    if(ydp->providerAuthStatus){

        providers->addProvider(ydp,true);
        providers->saveTokenFile("YandexDisk");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }
}


void listYandex(MSProvidersPool* providers){

    MSYandexDisk* ydp=new MSYandexDisk();

    ydp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(ydp,true);
    if(!providers->loadTokenFile("YandexDisk")){
        return ;
    }

    if(! providers->refreshToken("YandexDisk")){

        qStdOut()<< "Unauthorized client"<<endl;
        return;
    }

    ydp->readRemote("/");



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=ydp->syncFileList.begin();

    while(li != ydp->syncFileList.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}


void syncYandex(MSProvidersPool* providers){

    MSYandexDisk* ydp=new MSYandexDisk();

    ydp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(ydp);
    if(! providers->loadTokenFile("YandexDisk")){
        exit(0);
    }

    if(!providers->refreshToken("YandexDisk")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    ydp->createSyncFileList();

}


void infoYandex(MSProvidersPool* providers){

    MSYandexDisk* dbp=new MSYandexDisk();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp,true);
    if(!providers->loadTokenFile("YandexDisk")){
        return ;
    }

    if(! providers->refreshToken("YandexDisk")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }


    QString info=dbp->getInfo();

    if(info == "false"){

        qStdOut()<< "Error getting cloud information " <<endl;
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject job = json.object();

    double usage=  job["usage"].toString().toDouble();
    double total=  job["total"].toString().toDouble();
    double free= total - usage;

    qStdOut()<< job["account"].toString()<< endl << endl << "total: "<< (uint64_t)total <<endl << "usage: "<< (uint64_t)usage << endl << "free: "<< (uint64_t)free <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1048576<<" MB" <<endl << "usage: "<< (uint64_t)usage/1048576<<" MB"  << endl << "free: "<< (uint64_t)free/1048576<<" MB"  <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1073741824<<" GB" <<endl << "usage: "<< (uint64_t)usage/1073741824<<" GB"  << endl << "free: "<< (uint64_t)free/1073741824<<" GB"  <<endl;


}


void emptyTrashYandex(MSProvidersPool* providers){

    MSYandexDisk* ydp=new MSYandexDisk();

    ydp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(ydp);
    if(! providers->loadTokenFile("YandexDisk")){
        exit(0);
    }

    if(!providers->refreshToken("YandexDisk")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    ydp->remote_file_empty_trash();
}


void authMailru(MSProvidersPool* providers,QString login,QString password){

    MSMailRu* mrp=new MSMailRu();

    mrp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    mrp->login=login;
    mrp->password=password;

    mrp->auth();
    if(mrp->providerAuthStatus){

        providers->addProvider(mrp,true);
        providers->saveTokenFile("MailRu");
        qStdOut() << "Token was succesfully accepted and saved."<<endl;

    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }

    delete(mrp->cookies);
}


void listMailru(MSProvidersPool* providers){

    MSMailRu* mrp=new MSMailRu();

    mrp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(mrp,true);
    if(!providers->loadTokenFile("MailRu")){
        return ;
    }

    if(! providers->refreshToken("MailRu")){

        qStdOut()<< "Unauthorized client"<<endl;
        return;
    }

    mrp->readRemote("/",NULL);



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=mrp->syncFileList.begin();

    while(li != mrp->syncFileList.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}


void syncMailru(MSProvidersPool* providers){

    MSMailRu* mrp=new MSMailRu();

    mrp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(mrp);
    if(! providers->loadTokenFile("MailRu")){
        delete(mrp->cookies);
        exit(0);
    }

    if(!providers->refreshToken("MailRu")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        delete(mrp->cookies);
        exit(0);
    }

    mrp->createSyncFileList();

    delete(mrp->cookies);
}


void infoMailru(MSProvidersPool* providers){

    MSMailRu* dbp=new MSMailRu();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp,true);
    if(!providers->loadTokenFile("MailRu")){
        delete(dbp->cookies);
        return ;
    }

    if(! providers->refreshToken("MailRu")){

        delete(dbp->cookies);
        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }


    QString info=dbp->getInfo();

    if(info == "false"){

        qStdOut()<< "Error getting cloud information " <<endl;
        delete(dbp->cookies);
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject job = json.object();

    double usage=  job["usage"].toString().toDouble();
    double total=  job["total"].toString().toDouble();
    double free= total - usage;

    qStdOut()<< job["account"].toString()<< endl << endl << "total: n/d" <<endl << "usage: n/d" << endl << "free: n/d" <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total<<" MB" <<endl << "usage: "<< (uint64_t)usage<<" MB"  << endl << "free: "<< (uint64_t)free<<" MB"  <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1000<<" GB" <<endl << "usage: "<< (uint64_t)usage/1000<<" GB"  << endl << "free: "<< (uint64_t)free/1000<<" GB"  <<endl;


    delete(dbp->cookies);
}


void emptyTrashMailru(MSProvidersPool* providers){

    MSMailRu* mrp=new MSMailRu();

    mrp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(mrp);
    if(! providers->loadTokenFile("MailRu")){
        delete(mrp->cookies);
        exit(0);
    }

    if(!providers->refreshToken("MailRu")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        delete(mrp->cookies);
        exit(0);
    }

    mrp->remote_file_empty_trash();

    delete(mrp->cookies);
}


void authOneDrive(MSProvidersPool* providers){

    MSOneDrive* odp=new MSOneDrive();

    odp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    odp->auth();
    if(odp->providerAuthStatus){

        providers->addProvider(odp,true);
        providers->saveTokenFile("OneDrive");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }

}


void listOneDrive(MSProvidersPool* providers){

    MSOneDrive* gdp=new MSOneDrive();

    gdp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(gdp,true);
    if(!providers->loadTokenFile("OneDrive")){
        return ;
    }

    if(! providers->refreshToken("OneDrive")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }



    gdp->readRemote("");



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=gdp->syncFileList.begin();

    while(li != gdp->syncFileList.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}

void syncOneDrive(MSProvidersPool* providers){

    MSOneDrive* odp=new MSOneDrive();

    odp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(odp);
    if(! providers->loadTokenFile("OneDrive")){
        exit(0);
    }

    if(!providers->refreshToken("OneDrive")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    odp->createSyncFileList();
}


void infoOneDrive(MSProvidersPool* providers){

    MSOneDrive* dbp=new MSOneDrive();

    dbp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(dbp,true);
    if(!providers->loadTokenFile("OneDrive")){
        return ;
    }

    if(! providers->refreshToken("OneDrive")){

        qStdOut()<< "Unauthorized client"<<endl;
       return;
    }


    QString info=dbp->getInfo();

    if(info == "false"){

        qStdOut()<< "Error getting cloud information " <<endl;
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject job = json.object();

    double usage=  job["usage"].toString().toDouble();
    double total=  job["total"].toString().toDouble();
    double free= total - usage;

    qStdOut()<< job["account"].toString()<<" at OneDrive.com"<< endl << endl << "total: "<< (uint64_t)total <<endl << "usage: "<< (uint64_t)usage << endl << "free: "<< (uint64_t)free <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1048576<<" MB" <<endl << "usage: "<< (uint64_t)usage/1048576<<" MB"  << endl << "free: "<< (uint64_t)free/1048576<<" MB"  <<endl;
    qStdOut()<< "" << endl << "total: "<< (uint64_t)total/1073741824<<" GB" <<endl << "usage: "<< (uint64_t)usage/1073741824<<" GB"  << endl << "free: "<< (uint64_t)free/1073741824<<" GB"  <<endl;



}


void emptyTrashOneDrive(MSProvidersPool *providers){

    MSOneDrive* odp=new MSOneDrive();

    odp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

    providers->addProvider(odp);
    if(! providers->loadTokenFile("OneDrive")){
        exit(0);
    }

    if(!providers->refreshToken("OneDrive")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    odp->remote_file_empty_trash();
}


// ============= FUSE =======================

bool fuseMount(ProviderType prov,QString workPath,QString mountPoint){
Q_UNUSED(prov);
Q_UNUSED(workPath);
Q_UNUSED(mountPoint);
//    struct sockaddr_un name;
//    char buf[1000];

//    memset(&name, '0', sizeof(name));

//    name.sun_family = AF_UNIX;

//    snprintf(name.sun_path, 200, "%s", "/tmp/ccfd.sock");

//    int s=socket(PF_UNIX,SOCK_STREAM,0);

//    if(s== -1){
//        return false;
//    }

//    int r=connect(s, (struct sockaddr *)&name, sizeof(struct sockaddr_un));

//    if(r<0){
//        perror ("connect");
//        return false;
//    }

//    int sz= read(s,&buf[0],100);

//    if(sz >0){
//        QString reply=&buf[0];

//        if(reply.contains("HELLO")){

//            // try to start worker
//            snprintf(&buf[0], 1000, "%s^%d^%s^%s", "new_mount",prov,workPath.toStdString().c_str(),mountPoint.toStdString().c_str());
//            write(s,&buf[0],1000);

//            sz= read(s,&buf[0],100);

//            if(sz >0){

//                return true;
//            }
//            else{
//                return false;
//            }

//        }
//    }

    return false;
}






///////////////////////////////////////////////////////////////////////////////////


///
/// \brief getAIID - get Application Instance ID. Create AIID if no exist.
/// \return AIID
///
QString getAIID(){

    QString home=QDir::homePath() + QDir::separator() + CCROSS_HOME_DIR;
    QDir cc=QDir(home);

    if(!cc.exists()){
       cc.mkpath(home);
    }


    QString config=home + QDir::separator() + CCROSS_CONFIG_FILE;
    QFile key(config);
    QString aiid;

    if(!key.open(QIODevice::ReadOnly)){

        QJsonObject co;
        aiid=QUuid::createUuid().toString();
        co["AIID"]=aiid;
        QJsonDocument cd(co);


        bool r=key.open(QIODevice::WriteOnly | QIODevice::Text);
        if(r){
            QTextStream outk(&key);
            //outk << cd.toJson(QJsonDocument::Compact);
            outk << cd.toJson(QJsonDocument::Indented);
        }


    }
    else{
        QTextStream instream(&key);
        QString line;
        while(!instream.atEnd()){

            line+=instream.readLine();
        }

        QJsonObject job = QJsonDocument::fromJson(line.toUtf8()).object();
        aiid=job["AIID"].toString();
    }

    key.close();

    return aiid;


}


///
/// \brief getOS - get OS name
/// \return OS name
///
QString getOS(){
#if defined(Q_OS_ANDROID)
return QLatin1String("android");
#elif defined(Q_OS_BLACKBERRY)
return QLatin1String("blackberry");
#elif defined(Q_OS_IOS)
return QLatin1String("ios");
#elif defined(Q_OS_MAC)
return QLatin1String("macos");
#elif defined(Q_OS_WINCE)
return QLatin1String("wince");
#elif defined(Q_OS_WIN)
return QLatin1String("windows");
#elif defined(Q_OS_LINUX)
return QLatin1String("linux");
#elif defined(Q_OS_UNIX)
return QLatin1String("unix");
#else
return QLatin1String("unknown");
#endif
}

///////////////////////////////////////////////////////////////////////////////////






int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");


    QCoreApplication a(argc, argv);


    // Application instance definition

//    QString AAID=getAIID();
//    QString OS=getOS();

////    QSysInfo sy;

////    QStringList qv=QString(qVersion()).split(".");

//    QString PLATFORM;
//    QString DISTR;


////    if(qv.at(0).toInt()<5){
////        qDebug()<<"Qt version too low";
////        return 0;
////    }

////    if(qv.at(1).toInt()<4){

//        utsname u;
//        uname(&u);

//        PLATFORM=u.machine;
//        DISTR=QString(u.sysname)+" "+QString(u.release);


////    }
////    else{

////        PLATFORM=sy.currentCpuArchitecture();
////        DISTR=sy.productType()+" "+sy.productVersion();
////    }


////    MSRequest* req=new MSRequest();
////    req->setRequestUrl("http://cloudcross.mastersoft24.ru/stat");
////    req->setMethod("get");
////    req->addQueryItem("os",            OS);
////    req->addQueryItem("distr",         DISTR);
////    req->addQueryItem("platform",      PLATFORM);
////    req->addQueryItem("aaid",          AAID);

////    req->exec();


//    QTextCodec *russian =QTextCodec::codecForName("unicode");
//     QTextCodec::setCodecForLocale(russian);


    // create main objects

    MSProvidersPool* providers=new MSProvidersPool();


    QStringList opts=a.arguments();

    MSOptParser* parser=new MSOptParser();

    parser->insertOption(1,"--help -h  ");
    parser->insertOption(2,"-a --auth");
    parser->insertOption(3,"-v --version");
    parser->insertOption(4,"-p --path 1");
    parser->insertOption(7,"-s --list");
    parser->insertOption(8,"--use-include");

    parser->insertOption(5," --prefer 1"); // local or remote
    parser->insertOption(6,"--no-hidden");// don't sync hidden files and folders
    parser->insertOption(9,"--no-new-rev");// do not create new revision of file, overwrite him instead
    parser->insertOption(10,"--dry-run");
    parser->insertOption(11,"--convert-doc");
    parser->insertOption(12,"--force 1");
    parser->insertOption(13,"--provider 1"); // google, yandex, dropbox or mailru
//    parser->insertOption(14,"--direct-upload 2"); // upload file directly to cloud

    parser->insertOption(15,"--login 1");
    parser->insertOption(16,"--password 1");

    parser->insertOption(17,"--http-proxy 1");
    parser->insertOption(18,"--socks5-proxy 1");

    parser->insertOption(19,"--cloud-space");
    parser->insertOption(20,"--filter-type 1");

    parser->insertOption(20,"--fuse-mount 1");

    parser->insertOption(21,"--single-thread");

    parser->insertOption(22,"--low-memory");

    parser->insertOption(23,"--empty-trash");

    parser->insertOption(24,"--no-sync");

    //...............

    parser->parse(opts);


    int ret;

    ProviderType currentProvider;

    QStringList prov=parser->getParamByName("provider");



    if(prov.size() != 0){
        if(prov[0] == "google"){
            currentProvider=ProviderType::Google;
        }
        else if(prov[0] == "dropbox"){
            currentProvider=ProviderType::Dropbox;
        }
        else if(prov[0] == "yandex"){
            currentProvider=ProviderType::Yandex;
        }
        else if(prov[0] == "mailru"){
            currentProvider=ProviderType::Mailru;
        }
        else if(prov[0] == "onedrive"){
            currentProvider=ProviderType::OneDrive;
        }
        else {
            qStdOut()<< "Unknown cloud provider. Application terminated."<< endl;
            return 1;
        }
    }
    else{
        currentProvider=ProviderType::Google;
    }


    QStringList mailru_login;
    QStringList mailru_password;




    QStringList wp=parser->getParamByName("path");
    if(wp.size()==0){
        wp=parser->getParamByName("p");
    }

    if(wp.size()!=0){

        providers->setWorkPath(wp[0]);
    }


    //===================== FUSE SECTION ======================

    if(parser->isParamExist("fuse-mount")){

        fuseMount(currentProvider,providers->workPath,parser->getParamByName("fuse-mount")[0] );
        delete(providers);
        return 0;
    }

    //=========================================================



    // ATTENTION!! since v1.4.1 direct uploading is not supports
    if(parser->isParamExist("direct-upload")){

        qStdOut() << "Direct uploading is not supports. Terminate"<<endl;
        return 0;

        qStdOut() << "Start direct uploading..."<<endl;

        switch(currentProvider){

        case ProviderType::Google:{//------------------------------------------

            MSGoogleDrive* cp=new MSGoogleDrive();

            cp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

            providers->addProvider(cp);

            if(! providers->loadTokenFile("GoogleDrive")){
                delete(providers);
                return 1;
            }

            if(!providers->refreshToken("GoogleDrive")){
                qStdOut()<<"Unauthorized access. Aborted."<<endl;
                delete(providers);
                return 1;
            }

            QStringList p=parser->getParamByName("direct-upload");
            if(p.size()<2){
                qStdOut()<<"Option --direct-upload. Missing required argument"<<endl;
                return 1;
            }
            cp->directUpload(p[0],p[1]);

            qStdOut() << "Uploaded file was  stored in google:/"<< p[1]<<endl;
        }
            break;

        case ProviderType::Dropbox:{//----------------------------------------

            MSDropbox* cp=new MSDropbox();

            cp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

            providers->addProvider(cp);

            if(! providers->loadTokenFile("Dropbox")){
                delete(providers);
                return 1;
            }

            if(!providers->refreshToken("Dropbox")){
                qStdOut()<<"Unauthorized access. Aborted."<<endl;
                delete(providers);
                return 1;
            }


            QStringList p=parser->getParamByName("direct-upload");
            if(p.size()<2){
                qStdOut()<<"Option --direct-upload. Missing required argument"<<endl;
                delete(providers);
                return 1;
            }
            cp->directUpload(p[0],p[1]);

            qStdOut() << "Uploaded file was stored in dropbox:/"<< p[1]<<endl;
        }
            break;

        case ProviderType::Yandex:{//---------------------------------------

            MSYandexDisk* cp=new MSYandexDisk();

            cp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

            providers->addProvider(cp);

            if(! providers->loadTokenFile("YandexDisk")){
                delete(providers);
                return 1;
            }

            if(!providers->refreshToken("YandexDisk")){
                qStdOut()<<"Unauthorized access. Aborted."<<endl;
                delete(providers);
                return 1;
            }

            QStringList p=parser->getParamByName("direct-upload");
            if(p.size()<2){
                qStdOut()<<"Option --direct-upload. Missing required argument"<<endl;
                delete(providers);
                return 1;
            }
            cp->directUpload(p[0],p[1]);

            qStdOut() << "Uploaded file was stored in yandex:/"<< p[1]<<endl;
        }
            break;

        case ProviderType::Mailru:{//---------------------------------------

            MSMailRu* cp=new MSMailRu();

            cp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

            providers->addProvider(cp);

            if(! providers->loadTokenFile("MailRu")){
                delete(providers);
                return 1;
            }

            if(!providers->refreshToken("MailRu")){
                qStdOut()<<"Unauthorized access. Aborted."<<endl;
                delete(providers);
                return 1;
            }

            QStringList p=parser->getParamByName("direct-upload");
            if(p.size()<2){
                qStdOut()<<"Option --direct-upload. Missing required argument"<<endl;
                delete(providers);
                return 1;
            }
            cp->directUpload(p[0],p[1]);

            qStdOut() << "Uploaded file was stored in mail.ru:/"<< p[1]<<endl;
        }
            break;

        case ProviderType::OneDrive:{//---------------------------------------

            MSOneDrive* cp=new MSOneDrive();

            cp->setProxyServer(providers->proxyTypeString,providers->proxyAddrString);

            providers->addProvider(cp);

            if(! providers->loadTokenFile("OneDrive")){
                delete(providers);
                return 1;
            }

            if(!providers->refreshToken("OneDrive")){
                qStdOut()<<"Unauthorized access. Aborted."<<endl;
                delete(providers);
                return 1;
            }

            QStringList p=parser->getParamByName("direct-upload");
            if(p.size()<2){
                qStdOut()<<"Option --direct-upload. Missing required argument"<<endl;
                delete(providers);
                return 1;
            }
            cp->directUpload(p[0],p[1]);

            qStdOut() << "Uploaded file was stored in OneDrive:/"<< p[1]<<endl;
        }
            break;


        default:
            break;
        }

        qStdOut() << "Direct uploading completed"<<endl;

        delete(providers);

        return 0;
    }



    while((ret=parser->get())!=-1){
        switch(ret){

        case 1: // --help

            printHelp();
            return 0;
            //qStdOut()<< "HELP arg="+parser->optarg;
            break;

        case 2: //-- auth

            switch(currentProvider){
            case ProviderType::Google:
                authGrive(providers);
                break;
            case ProviderType::Dropbox:
                authDropbox(providers);
                break;
            case ProviderType::Yandex:
                authYandex(providers);
                break;
            case ProviderType::Mailru:

                mailru_login=parser->getParamByName("--login");
                mailru_password=parser->getParamByName("--password");

                if((mailru_login.size()==0)||(mailru_password.size()==0)){
                     qStdOut()<< "Provider Mail.ru. Login and password required. Application terminated."<< endl;
                     return 1;
                }


                authMailru(providers,mailru_login[0], mailru_password[0]);

                break;

            case ProviderType::OneDrive:
                authOneDrive(providers);

                break;

            default:
                break;
            }

            return 0;
            break;

        case 3: // --version

            printVersion();
            return 0;
            break;


        case 4: // --path

            providers->setWorkPath(parser->optarg[0]);
            break;

        case 5: // --prefer

            MSCloudProvider::SyncStrategy s;

            if(parser->optarg[0]=="local"){
                s=MSCloudProvider::SyncStrategy::PreferLocal;
            }

            else{
                if(parser->optarg[0]=="remote"){
                    s=MSCloudProvider::SyncStrategy::PreferRemote;
                }
                else{
                    qStdOut()<< "--prefer option value must be an one of \"local\" or \"remote\""<<endl;
                    return 0;
                    break;
                }
            }

            if(!parser->isParamExist("no-sync")){
                providers->setStrategy(s);
            }
            else{
                qStdOut()<< "--no-sync was defined. --prefer option will be ignored."<<endl;
            }
            break;

        case 7:// --list

            switch(currentProvider){
            case ProviderType::Google:
                listGrive(providers);
                break;
            case ProviderType::Dropbox:
                listDropbox(providers);
                break;
            case ProviderType::Yandex:
                listYandex(providers);
                break;
            case ProviderType::Mailru:
                listMailru(providers);
                break;
            case ProviderType::OneDrive:
                listOneDrive(providers);
                break;

            default:
                break;
            }

            return 0;
            break;

        case 6: // --no-hidden

            providers->setFlag("noHidden",true);
            break;

        case 8: // --use-include

            providers->setFlag("useInclude",true);
            break;

        case 9: // --no-new-rev

            if(currentProvider == ProviderType::Google){
                providers->setFlag("noNewRev",true);
            }
            else{
                qStdOut()<< "--no-new-rev option doesn't matter for this provider. "<<endl;
            }
            break;

        case 10: // --dry-run

            providers->setFlag("dryRun",true);
            break;

        case 11: // --convert-doc

            if(currentProvider == ProviderType::Google){
                providers->setFlag("convertDoc",true);
            }
            else{
                qStdOut()<< "--convert-doc option doesn't matter for this provider. "<<endl;
            }
            break;

        case 12: // --force

            if((parser->optarg[0]=="upload")||(parser->optarg[0]=="download")){
                providers->setOption("force",parser->optarg[0]);
                providers->setFlag("force",true);



                if(parser->optarg[0] == "download"){

                    providers->setStrategy(MSCloudProvider::SyncStrategy::PreferRemote);
                }
                else{

                    providers->setStrategy(MSCloudProvider::SyncStrategy::PreferLocal);
                }

            }
            else{
                qStdOut()<< "--force option value must be an one of \"upload\" or \"download\""<<endl;
                return 0;
                break;
            }
            break;

//        case 14:{

//            MSYandexDisk* cp=new MSYandexDisk();
//            providers->addProvider(cp);

//            if(! providers->loadTokenFile("YandexDisk")){
//                return 1;
//            }

//            if(!providers->refreshToken("YandexDisk")){
//                qStdOut()<<"Unauthorized access. Aborted."<<endl;
//                return 1;
//            }


//            cp->directUpload(parser->optarg[0],parser->optarg[1]);

//            return 0;
//            break;
//        }

        case 17:
                providers->proxyTypeString="http";
                providers->proxyAddrString=parser->optarg[0];

            break;

        case 18:

            providers->proxyTypeString="socks5";
            providers->proxyAddrString=parser->optarg[0];

            break;

        case 19: // --cloud-space

            switch(currentProvider){
                case ProviderType::Google:
                    infoGrive(providers);
                    break;
                case ProviderType::Dropbox:
                    infoDropbox(providers);
                    break;
                case ProviderType::Yandex:
                    infoYandex(providers);
                    break;
                case ProviderType::Mailru:
                    infoMailru(providers);
                    break;
                case ProviderType::OneDrive:
                    infoOneDrive(providers);
                    break;

                default:
                    break;

            }

            return 0;
            break;
        case 20: // --filter-type
            providers->setOption("filter-type",parser->optarg[0]);
            break;

        case 21: // --single-thread

            providers->setFlag("singleThread",true);
            break;

        case 22: // --low-memory

            providers->setFlag("lowMemory",true);
            break;

        case 23: // --empty-trash

            switch(currentProvider){
                case ProviderType::Google:
                    emptyTrashGrive(providers);
                    break;
                case ProviderType::Dropbox:
                    emptyTrashDropbox(providers);
                    break;
                case ProviderType::Yandex:
                    emptyTrashYandex(providers);
                    break;
                case ProviderType::Mailru:
                    emptyTrashMailru(providers);
                    break;
                case ProviderType::OneDrive:
                    emptyTrashOneDrive(providers);
                    break;

                default:
                    break;

            }

            return 0;
            break;

        case 24: // --no-sync

                providers->setFlag("noSync",true);
                providers->setStrategy(MSCloudProvider::SyncStrategy::PreferLocal);

            break;

        default: // syn execute without any params by default

            switch(currentProvider){
            case ProviderType::Google:
                syncGrive(providers);
                break;
            case ProviderType::Dropbox:
                syncDropbox(providers);
//                qStdOut()<< "sync dropbox"<<endl;
                break;
            case ProviderType::Yandex:
                syncYandex(providers);
//                qStdOut()<< "sync yandex"<<endl;
                break;
            case ProviderType::Mailru:
                syncMailru(providers);
//                qStdOut()<< "sync mailru"<<endl;
                break;
            case ProviderType::OneDrive:
                syncOneDrive(providers);
//                qStdOut()<< "sync onedrive"<<endl;
                break;

            default:
                break;
            }

            //exit(0);
            return 0;
            break;
        }
    }

    if(parser->erorrNum!=0){
        qStdOut()<< parser->errorString<<endl;
        return 1;
    }

    //return a.exec();
}
