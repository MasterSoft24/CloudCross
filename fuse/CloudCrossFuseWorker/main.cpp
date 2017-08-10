#include <signal.h>
#include <QCoreApplication>
#include <QTimer>
#include <QThread>

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <wchar.h>

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS  64
#define DAEMON_MSG_END "\n<---CCFD_MESSAGE_END-->"

#define WAIT_FOR_FIRST_DATA 7
#define WAIT_FOR_NEXT_DATA 6

#include <unistd.h>

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>

//#include "json.hpp"

#include "../libfusecc/libfusecc.h"
#include "../libfusecc/ccseparatethread.h"
#include "include/msproviderspool.h"

#include "cc_fusefs.h"




using namespace std;
//using json = nlohmann::json;

//#include "ccfw.h"
#include "incominglistener.cpp"
//#include "fswatcher.cpp"

static const char *filepath = "/file";
static const char *filename = "file";
static const char *filecontent = "I'm the content of the only file available there\n";

// forward declarations
void log(QString mes);

//string sendCommand_sync(json command);
//void sendCommand(json command);


// ========= GLOBALS ==========

static struct fuse_server {

    struct fuse_args args;

    pthread_t pid;

    pthread_t timer_pid;

    struct fuse *fuse;
    struct fuse_chan *ch;
    int failed;
    int running;
    char *mountpoint;
    int multithreaded;
    int foreground;
} fs;

static  fuse_operations fuse_op;

QString mountPath;
QString cachePath;
QString tokenPath;
ProviderType provider;
QString providerName;
//libFuseCC* ccLib;
//MSCloudProvider* providerObject;

pthread_t tid;

QString tempDirName;

// ----------------------------------------------------------------

void* onSyncTimer(void* p){

    while(true){

        sleep(5);



        int ct = QDateTime::currentSecsSinceEpoch();

        if( (CC_FuseFS::updateSheduled) && (!CC_FuseFS::fsBlocked) && ((ct - CC_FuseFS::lastUpdateSheduled) > 20)  ){

            CC_FuseFS::fsBlocked = true;

            CC_FuseFS::Instance()->log("SYNC started");

            QMap<QString,QVariant> p;
            p["cachePath"] = cachePath;

            // Get a copy of work object to pass it to the sync process. For read-only accesibility to mounted directory during sunc process working.
            MSCloudProvider* cp_c;
            CC_FuseFS::Instance()->ccLib->getProviderInstance(CC_FuseFS::provider, (MSCloudProvider**)&(cp_c), CC_FuseFS::tokenPath);
            cp_c->syncFileList = CC_FuseFS::Instance()->providerObject->syncFileList;
            cp_c->flags = CC_FuseFS::Instance()->providerObject->flags;
            cp_c->token = CC_FuseFS::Instance()->providerObject->token;
            cp_c->workPath = CC_FuseFS::Instance()->providerObject->workPath;
            cp_c->credentialsPath = CC_FuseFS::Instance()->providerObject->credentialsPath;

//            CC_FuseFS::Instance()->ccLib->run(CC_FuseFS::Instance()->providerObject,"sync", p);
            CC_FuseFS::Instance()->ccLib->run(cp_c,"sync", p);

            CC_FuseFS::updateSheduled = false;
            CC_FuseFS::fsBlocked = false;
            delete(cp_c);
        }

    }

}

// ----------------------------------------------------------------

int readFileContent(char* path,char* buf,int size, int offset){



}

// ----------------------------------------------------------------

int f_mknod(const QString &path){
    QString f_path;
    int r= system(QString("touch \""+path+"\"").toStdString().c_str());
    if(r == 0){

        if(path.indexOf(cachePath) == 0){
            f_path = QString(path).remove(0,cachePath.length());
        }
        else{
            f_path = path;
        }


         CC_FuseFS::Instance()->ccLib->readSingleLocalFile(CC_FuseFS::Instance()->providerObject,QString(path),cachePath);
         QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(f_path));
         i.value().state = MSFSObject::ObjectState::NewLocal;


        return r;
    }
    else{
        return -1;
    }
}


// ----------------------------------------------------------------


int f_mkdir(const QString &path){
    QString f_path;
    int r= system(QString("mkdir -p \""+path+"\"").toStdString().c_str());
    if(r == 0){

        if(path.indexOf(cachePath) == 0){
            f_path = QString(path).remove(0,cachePath.length());
        }
        else{
            f_path = path;
        }

        CC_FuseFS::Instance()->ccLib->readSingleLocalFile(CC_FuseFS::Instance()->providerObject,QString(path),cachePath);
        QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(f_path));
        i.value().state = MSFSObject::ObjectState::NewLocal;

        return r;
    }
    else{
        return -1;
    }
}

// ----------------------------------------------------------------

int f_rmdir(const QString &path){

    QString f_path;
    QDir d(path);

    if(d.exists()){
        bool r = d.removeRecursively();

        CC_FuseFS::Instance()->ccLib->clearLocalPartOfSyncFileList(CC_FuseFS::Instance()->providerObject);
        CC_FuseFS::Instance()->ccLib->readLocalFileList(CC_FuseFS::Instance()->providerObject,cachePath);
        return 0;
    }


    //int r= system(QString("rm -rf \""+path+"\"").toStdString().c_str());

    else{
        f_path = QString(path).remove(0,cachePath.length());

        QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(f_path));
        if( i  != CC_FuseFS::Instance()->providerObject->syncFileList.end() ){

            i.value().state = MSFSObject::ObjectState::DeleteLocal;
            return 0;
        }
        else{
            return -1;
        }



    }
}


// ----------------------------------------------------------------

string getTempName() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string rnd;
    srand (time(NULL));

    for (int i = 0; i < 7; ++i) {
        rnd+= alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return rnd+"-CCF.temp";
}




// ----------------------------------------------------------------

//MSFSObject convertJsonToFS_remote(const json &jo){

//    MSFSObject obj;

//    obj.state               = jo["state"];
//    obj.path                = jo["path"];
//    obj.fileName            = jo["fileName"];
//    obj.refCount            = -1;

//    //obj.remote.data         = jo["remote"]["data"];
//    obj.remote.exist        = true;
//    obj.remote.fileSize     = jo["remote"]["fileSize"];
//    obj.remote.md5Hash      = jo["remote"]["md5Hash"];
//    obj.remote.modifiedDate = jo["remote"]["modifiedDate"];
//    obj.remote.objectType   = jo["remote"]["objectType"];

//    return obj;
//}

// ----------------------------------------------------------------

//bool findObjectInFilelist(const string &path, MSFSObject* obj ){

////    map<string, MSFSObject>::iterator i= fileList.find(string(path)); //does try find in local filelist

////    if( i != fileList.end() ){// if it was finded in local filelist

////        obj = &i->second;

////        return true;
////    }
////    else{

//        json comm;
//        comm["command"] = "find_object";
//        comm["params"]["socket"] = workSocket;
//        comm["params"]["provider"] = workProvider;
//        comm["params"]["path"] = workPath;
//        comm["params"]["mount"] = mountPoint;
//        comm["params"]["objectPath"] = path;

//        json jsonFileList = json::parse( sendCommand_sync(comm) );

//        if(jsonFileList["code"] == 0){

//            if(obj == 0){
//                return true;
//            }

//            json jo = jsonFileList["data"];

//            obj->state               = jo["state"];
//            obj->path                = jo["path"];
//            obj->fileName            = jo["fileName"];
//            obj->refCount            = -1;

//            //obj.remote.data         = jo["remote"]["data"];
//            obj->remote.exist        = true;
//            obj->remote.fileSize     = jo["remote"]["fileSize"];
//            obj->remote.md5Hash      = jo["remote"]["md5Hash"];
//            obj->remote.modifiedDate = jo["remote"]["modifiedDate"];
//            obj->remote.objectType   = jo["remote"]["objectType"];

//            return true;
//        }
//        else{


//            return false;
//        }

////    }




//}

// ----------------------------------------------------------------

QHash<QString, MSFSObject> filterListByPath(/*map<string, MSFSObject> src,*/ QString path){

    QHash<QString, MSFSObject> fl;

        for(QHash<QString, MSFSObject>::iterator i = CC_FuseFS::Instance()->providerObject->syncFileList.begin(); i != CC_FuseFS::Instance()->providerObject->syncFileList.end(); i++){

            if((path == i.value().path) && (i.value().state != MSFSObject::ObjectState::DeleteLocal)){

                fl.insert(i.key(),i.value());

            }
        }

    return fl;
}


// ----------------------------------------------------------------

static int fgetattr_callback(const char *path, struct stat *stbuf, struct fuse_file_info * fi) {

//log("STUB fgetattr");
return -ENOENT;
}

// ----------------------------------------------------------------

static int getattr_callback(const char *path, struct stat *stbuf) {

    if (QString(path) == QString("/")){
        log("CALLBACK getattr for /");

              stbuf->st_mode = S_IFDIR | 0755;
              stbuf->st_uid=getuid();
              stbuf->st_gid=getgid();
              stbuf->st_nlink = 2;
              return 0;
    }
    else{



        QString fname = QString("/tmp/")+tempDirName+QString(path);

        QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

        if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

            MSFSObject o = i.value();

            if( i.value().state == MSFSObject::ObjectState::NewRemote){ // not cached

                memset(stbuf,0,sizeof(struct stat));

                if(o.remote.objectType == MSRemoteFSObject::Type::folder){

                    stbuf->st_mode = S_IFDIR | 0755;
                    stbuf->st_nlink = 2;
                    //stbuf->st_size = 4096;
                }
                else{

                    stbuf->st_mode = S_IFREG | 0777;
                    stbuf->st_nlink = 1;
                    stbuf->st_size = o.remote.fileSize;
                }

                stbuf->st_uid=getuid();
                stbuf->st_gid=getgid();
                stbuf->st_mtime=o.remote.modifiedDate/1000;

                return 0;


            }

            if(stat(fname.toStdString().c_str(), stbuf) == 0){// if it was localy created but already synced

                return 0;
            }

            if( (i.value().state == MSFSObject::ObjectState::NewLocal) ||
                (i.value().state == MSFSObject::ObjectState::ChangedLocal)){

                int res;
                res = stat(fname.toStdString().c_str(), stbuf);


                if (res == -1){
                        return -errno;
                }

                return 0;
            }
        }
        else{

        }


        return -ENOENT;







        //string findname=string(path)+"/";
        log("CALLBACK getattr "+fname);

        struct stat buffer;
        if((stat (fname.toStdString().c_str(), &buffer) != 0)  ){//file not cached

            log("CALLBACK getattr:  file not cached");





                if( i != CC_FuseFS::providerObject->syncFileList.end() ){// file or folder exists on remote

                    MSFSObject o = i.value();

                //if(i != fileList.end()){// file or folder exists on remote

                    log("CALLBACK getattr:  file or folder exists on remote");

                    //MSFSObject o= i->second;

                    memset(stbuf,0,sizeof(struct stat));

                    if(o.remote.objectType == MSRemoteFSObject::Type::folder){

                        stbuf->st_mode = S_IFDIR | 0755;
                        stbuf->st_nlink = 2;
                        //stbuf->st_size = 4096;
                    }
                    else{

                        stbuf->st_mode = S_IFREG | 0777;
                        stbuf->st_nlink = 1;
                        stbuf->st_size = o.remote.fileSize;
                    }

                    stbuf->st_uid=getuid();
                    stbuf->st_gid=getgid();
                    stbuf->st_mtime=o.remote.modifiedDate/1000;

                    return 0;
                }
                else{ // this is new file or folder
                    log("CALLBACK getattr:  this is new file or folder");

//                    stbuf->st_mode = S_IFREG | 0777;
//                    stbuf->st_nlink = 1;
//                    stbuf->st_size = 0;
//                    stbuf->st_uid=1000;
//                    stbuf->st_gid=1000;
//                    stbuf->st_mtime=time(NULL);

                    //stat (fname.c_str(), stbuf);

                   // log("CALLBACK getattr NEW FILE "+to_string(stbuf->st_mode));
                   //memset(stbuf, 0, sizeof(stbuf));
//                    stbuf->st_mode = S_IFREG | 0777;
//                    stbuf->st_uid = getuid();
//                    stbuf->st_gid = getgid();
//                    stbuf->st_atime = stbuf->st_mtime = time(NULL);

                    return -ENOENT;
                    //return 0;
                }

              return 0;
        }
        else{// file cached



            log("CALLBACK getattr:  file cached");
            int res;
            res = stat(fname.toStdString().c_str(), stbuf);


            if (res == -1){
                    return -errno;
            }
            //log("CALLBACK getattr CACHED FILE SIZE IS "+to_string(stbuf->st_size));
            return 0;

        }


    }

    return -ENOENT;
}

// ----------------------------------------------------------------


static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){

    (void) offset;
    (void) fi;
    log("CALLBACK readdir "+QString(path));




     QString m_path;

     if(QString(path) == "/"){

         m_path=QString(path);
     }
     else{
        m_path=QString(path)+"/";
     }


    QHash<QString, MSFSObject> dl= filterListByPath( /*fileList,*/ QString(m_path));

    log("CALLBACK readdir  COUNT OF DIR ENT IS "+QString::number(dl.size()));

    if(  dl.size() != 0 ){

        QHash<QString, MSFSObject>::iterator i=dl.begin();

        for(;i != dl.end();i++){

            struct stat st;
            memset(&st, 0, sizeof(struct stat));

            if(i.value().remote.objectType == MSRemoteFSObject::Type::folder){

                st.st_mode = S_IFDIR | 0755;
                st.st_nlink = 2;
            }
            else{

                st.st_mode = S_IFREG | 0777;
                st.st_nlink = 1;
                st.st_size = i.value().remote.fileSize;
            }

            st.st_uid=getuid();
            st.st_gid=getgid();
            st.st_mtime=i.value().remote.modifiedDate/1000;


            filler(buf,i.value().fileName.toStdString().c_str(),&st,0);

        }

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        return 0;
    }
    else{
        struct stat st;
//        memset(&st, 0, sizeof(struct stat));
//        st.st_mode = S_IFDIR | 0755;
//        st.st_nlink = 2;
//        st.st_uid=getuid();
//        st.st_gid=getgid();
//        filler(buf,".",&st,0);

        memset(&st, 0, sizeof(struct stat));
        st.st_mode = S_IFDIR | 0755;
        st.st_nlink = 2;
        st.st_uid=getuid();
        st.st_gid=getgid();
        filler(buf,"..",&st,0);

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        return -1;
    }

    //return -1;


}

static int open_callback(const char *path, struct fuse_file_info *fi) {

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


//log("OPEN_CALLBACK for "+QString(path));
//char* argv[1];
//int argc=1;
//argv[0]="this";
//QCoreApplication ca(argc, argv);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    //MSFSObject o;

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

    //if(i != fileList.end()){

        QString fname=QString("/tmp/")+tempDirName+QString(path);

        struct stat buffer;
        if( stat(fname.toStdString().c_str(), &buffer) == 0){//file exists in cache

            //log("CALLBACK open - File Exists "+string(path)+" flags = "+to_string(fi->flags));

            //i->second.refCount++;

            fi->fh = open (fname.toStdString().c_str() ,fi->flags);
            if(fi->fh == -1){
                //log("CALLBACK open EXISTS - open for writing error");
            }

            //log("OPEN_CALLBACK File Exists "+fname);
            return 0;

        }
        else{// file is missing in cache

           //log("OPEN_CALLBACK File Missing "+fname);

            QString ptf=fname.mid(0,fname.lastIndexOf("/"));

            system(QString(QString("mkdir -p \"")+ptf+QString("\"")).toStdString().c_str());

            // create zero-size file
            //log("Create ZERO-SIZED file "+QString(path));
            fstream fs;
            fs.open(fname.toStdString().c_str(), ios::out);
            fs.close();

            fi->fh = open (fname.toStdString().c_str() ,fi->flags);
            if(fi->fh == -1){
                //log("CALLBACK open MISSING - open for writing error");
            }

            QMap<QString,QVariant> p;
            //p.insert("path",QVariant(tokenPath));
            p.insert("cachePath",QVariant(fname));
            p.insert("filePath",QVariant(QString(path)));

            //ccLib->runInSeparateThread(providerObject,QString("get_content"),p);
            CC_FuseFS::Instance()->ccLib->runInSeparateThread(CC_FuseFS::Instance()->providerObject,QString("get_content"),p);

            //log("OPEN_CALLBACK File Will Be Downloaded "+fname);

//            json comm;
//            comm["command"]="get_content";
//            comm["params"]["socket"]=workSocket;
//            comm["params"]["provider"]=workProvider;
//            comm["params"]["path"]=workPath;
//            comm["params"]["cachePath"]=fname;
//            comm["params"]["filePath"]=string(path);

//            sendCommand(comm) ;

            //i->second.refCount=0;
        }

        //log("open callback ended for "+string(path)+" ; ref count is "+to_string(i->second.refCount));
        return 0;
    }

    //log("OPEN_CALLBACK This File Don't Exists On Remote "+QString(path));
  return -1;
}


// ----------------------------------------------------------------

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    //if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    MSFSObject o;
    int fullSize;

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        o = i.value();
    }
    else{
        return -1;
    }


    if( (o.state == MSFSObject::ObjectState::NewLocal)  || (o.state == MSFSObject::ObjectState::ChangedLocal) ){

        fullSize = o.local.fileSize;
    }
    else{
        fullSize = o.remote.fileSize;
    }



    QString fname = QString("/tmp/")+tempDirName+QString(path);
    log("READ_CALLBACK for "+fname);

    struct stat buffer;
    if(stat(fname.toStdString().c_str(), &buffer) == 0){//file exists in cache

//        // get real size of file
//        FILE* r_f=fopen(fname.c_str(),"rb");
//        fseek(r_f,0,SEEK_END);
//        buffer.st_size = ftell(r_f);
//        fclose(r_f);


        if(buffer.st_size == 0){// file don't cached yet

            if(false){

                log("CALLBACK read - file LOCAL CREATED and fully cached "+QString(path));

                FILE* f = fopen(fname.toStdString().c_str(),"rb");
                fseek(f,offset,SEEK_SET);
                size_t cnt = fread(buf,1,size,f);
                fclose(f);

                return cnt;
            }
            else{

                log("CALLBACK read - file exists but don't cached "+QString(path));

                for(int i=0;i < WAIT_FOR_FIRST_DATA;i++){
                    sleep(1);
                    stat (fname.toStdString().c_str(), &buffer);
                    if( (buffer.st_size == fullSize) || (buffer.st_size >= size) ){

                        return read_callback(path,buf,size,offset,fi);
                    }
                }

                return 0;
            }

        }
        else{// fully or partialy cached

            if(fullSize == buffer.st_size){// fully cached


//                if(offset > buffer.st_size){
//                    log("CALLBACK read - (offset > buffer.st_size) "+string(path));
//                    return 0;
//                }

//                if(offset + size > buffer.st_size){

//                    log("CALLBACK read - (offset + size > buffer.st_size) "+string(path));

//                    FILE* f = fopen(fname.c_str(),"rb");
//                    fseek(f,offset,SEEK_SET);
//                    //size_t cnt = fread(buf,1,(buffer.st_size - offset),f);
//                    size_t cnt = fread(buf,1,(size),f);
//                    fclose(f);
//                    return (int)size;//(buffer.st_size - offset);
//                    //return 0;
//                }


//                log("CALLBACK read - NORM "+string(path));
//                FILE* f = fopen(fname.toStdString().c_str(),"rb");
//                fseek(f,offset,SEEK_SET);
//                size_t cnt = fread(buf,1,size,f);
//                fclose(f);

//                if(cnt != size){
//                    return (int)cnt;
//                }
//                else{
//                    return (int)size;
//                }


                    //log("CALLBACK read - NEW "+to_string(fullSize)+" ("+to_string(buffer.st_size)+") "+to_string(offset)+" "+to_string(size));

                    ifstream data;
                    data.open(fname.toStdString().c_str(),   ios::in | ios::binary);//ios::out |ios::trunc |

                    data.seekg(static_cast<streamoff>(offset));
                    data.read(buf, static_cast<streamoff>(size));
                    if (data.eof()){ // read may fail if we reach eof

                        data.clear(); // clear possible failbit
                    }

                    log("CALLBACK read - NEW READED "+QString::number(data.gcount()));

                    return static_cast<int>(data.gcount());




//                log("CALLBACK read - file exists and fully cached "+string(path));

//                FILE* f = fopen(fname.c_str(),"rb");
//                fseek(f,offset,SEEK_SET);
//                size_t cnt = fread(buf,1,size,f);
//                //fclose(f);

//                if(cnt < 0){
//                    fclose(f);
//                    return 0;
//                }

//                if(cnt != size){
//                    int y;
//                    if(cnt > size){
//                        y = cnt - size;
//                        log("CALLBACK read - ERROR OCCURED (CNT is greater) SIZE MISSMATCH IS  "+to_string(y));
//                        fclose(f);
//                        return size;
//                    }
//                    else{

//                        y = size - cnt;
//                        log("CALLBACK read - ERROR OCCURED (SIZE is greater) SIZE MISSMATCH IS  "+to_string(y));

//                        fclose(f);
//                        return cnt;

//                    }
//                    //log("CALLBACK read - ERROR OCCURED SIZE MISSMATCH IS  "+to_string(y));

//                    fclose(f);
//                }

//                log("CALLBACK read - WAS READED  "+to_string(cnt));
//                return cnt;
            }
            else{// partialy cached

               // log("CALLBACK read - file exists and partially cached "+string(path));

                int endPosition=size+offset-1;

                if(endPosition > fullSize){
                    endPosition = fullSize;
                }

                for(int i=0;i < WAIT_FOR_NEXT_DATA;i++){
                    sleep(1);
                    stat (fname.toStdString().c_str(), &buffer);
                    if( (endPosition <= buffer.st_size) ){

                        FILE* f = fopen(fname.toStdString().c_str(),"rb");
                        fseek(f,offset,SEEK_SET);
                        size_t cnt = fread(buf,1,size,f);
                        fclose(f);

                        return cnt;

                    }

                    return 0;
                }



                if(offset < buffer.st_size){// i can read at least few bytes

                }
                else{// required file range don't cached. need wait

                }

            }

        }

    }
    else{

        //log("CALLBACK read - file DON'T exist "+string(path));
        open_callback(path,fi);
        return 0;
    }


return -ENOENT;
}

// ----------------------------------------------------------------

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

   // log("CALLBACK write");
    QString fname = QString("/tmp/")+tempDirName+QString(path);


    int fd;
    int res;
    (void) fi;
    fd = open(fname.toStdString().c_str(), O_WRONLY);
    if (fd == -1){
            return -errno;
    }

    res = pwrite(fd, buf, size, offset);

    if (res == -1){
        //log("write callback ERROR");
        close(fd);

        return -errno;
    }
    close(fd);

    struct stat buffer;
    stat (fname.toStdString().c_str(), &buffer);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        MSFSObject o = i.value();

        i.value().remote.fileSize=buffer.st_size;
        i.value().state = MSFSObject::ObjectState::ChangedLocal;
        CC_FuseFS::Instance()->doSheduleUpdate();
    }

    return res;

}


static int ftruncate_callback(const char *path, off_t size,struct fuse_file_info *fi)
{
    (void)fi;
    //log("CALLBACK ftruncate");

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


    QString fname = QString("/tmp/")+tempDirName+QString(path);


        int res;
        res = truncate(fname.toStdString().c_str(), size);
        if (res == -1){
                return -errno;
        }


        return 0;
}



static int truncate_callback(const char *path, off_t size)
{

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

    //log("CALLBACK truncate");
    QString fname = QString("/tmp/")+tempDirName+QString(path);


        int res;
        res = truncate(fname.toStdString().c_str(), size);
        if (res == -1){
                return -errno;
        }


        return 0;
}


static int xmp_readlink(const char *path, char *buf, size_t size){

    //log("STUB: readlink");
    return 0;
}


static int xmp_mknod(const char *path, mode_t mode, dev_t rdev){


    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


    QString fname = QString("/tmp/")+tempDirName+QString(path);
    QString insname=QString(path);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        return -errno;
    }

    log("CALLBACK mknod begin"+fname);

    int res = f_mknod(fname);
//    int res = f_mknod(QString(path));

    if(res == -1){


        QString td = QString("/tmp/")+tempDirName;
        QString ptf = fname.mid(0,fname.lastIndexOf("/"));


        log("CALLBACK mknod failed. Try create dir"+ptf);

        res = f_mkdir(ptf);
        if(res == -1){
            log("CALLBACK mknod create dir failed. ");
            return -errno;
        }
        else{
            res = f_mknod(fname);
//            res = f_mknod(QString(path));
            if(res == -1){
                log("CALLBACK mknod  failed again.");
                return -errno;
            }
        }


    }


//    CC_FuseFS::Instance()->ccLib->readSingleLocalFile(CC_FuseFS::Instance()->providerObject,QString(path));

//    i.value().state = MSFSObject::ObjectState::NewLocal;

    CC_FuseFS::Instance()->doSheduleUpdate();

    return 0;
}


static int xmp_mkdir(const char *path, mode_t mode){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

    log("CALLBACK mkdir "+QString(path));

    QString fname = QString("/tmp/")+tempDirName+QString(path);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        return -errno;
    }

        int res = f_mkdir(fname);
        if(res == -1){
            return -errno;
        }
        else{

            //CC_FuseFS::Instance()->ccLib->readSingleLocalFile(CC_FuseFS::Instance()->providerObject,QString(path));

            CC_FuseFS::Instance()->doSheduleUpdate();

            return 0;
        }
}


static int xmp_unlink(const char *path){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

   // log("CALLBACK unlink");
    QString fname = QString("/tmp/")+tempDirName+QString(path);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    int res = unlink(fname.toStdString().c_str());

    //CC_FuseFS::Instance()->providerObject->syncFileList.remove(i.key());

    i.value().state = MSFSObject::ObjectState::DeleteLocal;

    CC_FuseFS::Instance()->doSheduleUpdate();

    return 0;
}


static int xmp_rmdir(const char *path){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

//    //log("CALLBACK: rmdir");
    QString fname = QString("/tmp/")+tempDirName+QString(path);

    int r = f_rmdir(fname);

//    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));


//    int res = rmdir(fname.toStdString().c_str());

//    i.value().state = MSFSObject::ObjectState::DeleteLocal;

    CC_FuseFS::Instance()->doSheduleUpdate();

    return r;
}


static int xmp_symlink(const char *from, const char *to){

    //log("STUB: symlink");
        return 0;
}


static int xmp_rename(const char *from, const char *to){


    return -1;

#ifdef THIS_IS_STUB
    QString fname_from = QString("/tmp/")+tempDirName+QString(from);
    QString fname_to = QString("/tmp/")+tempDirName+QString(to);


    QHash<QString, MSFSObject>::iterator i_to= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(to));

    if( i_to != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        return -errno; // name exists
    }

    QHash<QString, MSFSObject>::iterator i_from= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(from));

    if((i_from.value().remote.objectType == MSRemoteFSObject::Type::folder) || (i_from.value().local.objectType == MSLocalFSObject::Type::folder)){

    }
    else{

        CC_FuseFS::Instance()->providerObject->syncFileList.insert(QString(to),i_from.value());
        i_to= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(to));
        i_to.value().fileName = QFileInfo(to).fileName();
        i_to.value().state = MSFSObject::ObjectState::NewLocal;

        i_from.value().state = MSFSObject::ObjectState::DeleteLocal;

        rename( fname_from.toStdString().c_str() , fname_to.toStdString().c_str() );

    }

    return 0;
#endif
}



static int xmp_link(const char *from, const char *to){

   // log("STUB: link");
        return 0;
}


static int xmp_chmod(const char *path, mode_t mode){

    //log("STUB: chmod");
        return 0;
}



static int xmp_chown(const char *path, uid_t uid, gid_t gid){

    //log("STUB: chown");
        return 0;
}



static int xmp_statfs(const char *path, struct statvfs *stbuf){

//    int res;
    QString fname = QString("/tmp/")+tempDirName+QString(path);
//log("CALLBACK statfs "+fname);
//    res = statvfs(fname.c_str(), stbuf);
//    if (res == -1)
//            return -errno;
//    return 0;


//stbuf->f_fsid = {}; // ignored
//stbuf->f_bsize = 4096; // a guess!
//stbuf->f_blocks = 1000 * 256 ; // * 1024 * 1024 / f_bsize
//stbuf->f_bfree = stbuf->f_blocks - 2 * 256;
//stbuf->f_bavail = stbuf->f_bfree;
//stbuf->f_namemax = 256;
return 0;

}



static int xmp_access(const char *path, int mask){

    int res;
    QString fname = QString("/tmp/")+tempDirName+QString(path);
    //log("CALLBACK access "+fname);

    res = access(fname.toStdString().c_str(), mask);
    if (res == -1)
            return -errno;
    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{

    (void) path;
    (void) fi;

//    //log("CALLBACK release");

//    QString fname = QString("/tmp/")+tempDirName+QString(path);

//    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

//    //MSFSObject o;

//    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

//        QFile f(fname);
//        //f.remove();
//        f.close();

//    }


        return 0;
}



static int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi){

        //log("CALLBACK fsync");
        (void) path;
        (void) isdatasync;
        (void) fi;
        return 0;
}
// ----------------------------------------------------------------


static void destroy_callback(void* d){
    log("CALLBACK destroy");

    system(QString("rm -rf /tmp/"+tempDirName).toStdString().c_str());

    pthread_cancel(fs.pid);
    pthread_cancel(fs.timer_pid);
    kill(getpid(), SIGTERM);



}


// ----------------------------------------------------------------


// ----------------------------------------------------------------

void log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%T - ",timeinfo);

        mes = QString(buffer)+mes+" \n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

//    string ns="echo "+mes+" >> /tmp/ccfw.log ";
//    system(ns.c_str());
    return;
}


void* fuseLoop(void* p){

    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

    char* a[2];
    a[0] = "//usr/bin/true";//argv[0];
    a[1] = (char*)mountPath.toStdString().c_str(); //argv[4];
//    a[2]="-s";//
//    a[3]="-f";//

    fs.args = FUSE_ARGS_INIT(2, a);

    int res;


    fs.mountpoint = "/tmp/example";
    fs.multithreaded = true;
    fs.foreground = true;

    res = fuse_parse_cmdline(&fs.args, &fs.mountpoint, &fs.multithreaded, &fs.foreground);

    //fuse_unmount(fs.mountpoint, fs.ch);

    fs.ch = fuse_mount(fs.mountpoint, &fs.args);

    //fuse_daemonize(fs.foreground);

    if(!fs.ch) {

        return nullptr;
    }

    fs.fuse = fuse_new(fs.ch, &fs.args, &fuse_op, sizeof(fuse_op), NULL);


    struct fuse_session *se = fuse_get_session(fs.fuse);

    if (fuse_set_signal_handlers(se) != 0) {
       return nullptr;
    }


    //res = fuse_loop(fs.fuse/*, opts.clone_fd*/);
    res = fuse_loop_mt(fs.fuse/*, args.clone_fd*/);

    fuse_remove_signal_handlers(se);
    out3:
        fuse_unmount(fs.mountpoint,fs.ch);
    out2:
        fuse_destroy(fs.fuse);
    out1:
        //free(opts.mountpoint);
        fuse_opt_free_args(&fs.args);

}



// ----------------------------------------------------------------

int main(int argc, char *argv[])
{
    // argv[0]
    // argv[1] = socket name for communicate with command server
    // argv[2] = provider code
    // argv[3] = path to credentials
    // argv[4] = mount point



   QCoreApplication ca(argc, argv);

   int* rv,rc;
   pthread_attr_t attr;
   rc=pthread_attr_init(&attr);


   rc=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


    rc=pthread_create(&(fs.timer_pid), NULL, &onSyncTimer, NULL);

    rc=pthread_detach(fs.timer_pid);

//    pthread_join(tid,(void**)&rv);

    tokenPath = "/home/ms/QT/CloudCross/build/debug/TEST4";  // argv[3]
    mountPath = "/tmp/example"; // argv[4]

    tempDirName="TESTING_TEMPORARY";//string(argv[1])+string("_")+getTempName();
    cachePath = "/tmp/"+tempDirName;

    provider = (ProviderType) QString("0").toInt(); //QString::number(argv[2])

    //ccLib = new libFuseCC();

    CC_FuseFS::tokenPath = "/home/ms/QT/CloudCross/build/debug/TEST4";  // argv[3]
    CC_FuseFS::mountPath = "/tmp/example"; // argv[4]

    CC_FuseFS::provider = (ProviderType) QString("4").toInt(); //QString::number(argv[2])

    CC_FuseFS::Instance()->ccLib = new libFuseCC();


    bool lf_r = CC_FuseFS::Instance()->ccLib->getProviderInstance(CC_FuseFS::provider, (MSCloudProvider**)&(CC_FuseFS::providerObject), CC_FuseFS::tokenPath);
    CC_FuseFS::providerObject->workPath = cachePath;

    lf_r = CC_FuseFS::Instance()->ccLib->readRemoteFileList(CC_FuseFS::providerObject);



    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

    char* a[2];
    a[0] = argv[0];
    a[1] = (char*)mountPath.toStdString().c_str(); //argv[4];
//    a[2]="-s";//
//    a[3]="-f";//


    fuse_op.getattr = getattr_callback;
    //fuse_op.fgetattr = fgetattr_callback;
    fuse_op.open = open_callback;
    fuse_op.read = read_callback;
    fuse_op.readdir = readdir_callback;
    fuse_op.destroy = destroy_callback;
    fuse_op.write = write_callback;
    fuse_op.ftruncate = ftruncate_callback;
    fuse_op.truncate = truncate_callback;
    fuse_op.release= xmp_release;
    fuse_op.fsync = xmp_fsync;

    fuse_op.readlink = xmp_readlink;
    fuse_op.mknod = xmp_mknod;
    fuse_op.mkdir = xmp_mkdir;
    fuse_op.unlink = xmp_unlink;
    fuse_op.rmdir = xmp_rmdir;
    fuse_op.symlink= xmp_symlink;
    fuse_op.rename = xmp_rename;
    fuse_op.link = xmp_link;
    fuse_op.chmod = xmp_chmod;
    fuse_op.chown = xmp_chown;
    fuse_op.statfs = xmp_statfs;

    fuse_op.access = xmp_access;





    mkdir(QString("/tmp/"+tempDirName).toStdString().c_str(),0777);


    int* pp;
    pthread_attr_t f_attr;
    rc=pthread_attr_init(&f_attr);


    rc=pthread_attr_setdetachstate(&f_attr, PTHREAD_CREATE_DETACHED);


     rc=pthread_create(&(fs.pid), NULL, &fuseLoop, NULL);

     //rc=pthread_detach(fs.pid);
     pthread_join(fs.pid,(void**)&pp);


//    fuse_main(2, a, &fuse_op, NULL);

    return 0;;//ca.exec();
}



