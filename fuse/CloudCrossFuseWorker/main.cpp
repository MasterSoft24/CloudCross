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


#include "../libfusecc/libfusecc.h"
#include "../libfusecc/ccseparatethread.h"
#include "include/msproviderspool.h"

#include "cc_fusefs.h"





using namespace std;


// forward declarations
void log(QString mes);


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

pthread_t tid;

QString tempDirName;

// ----------------------------------------------------------------

void* onSyncTimer(void* p){

    Q_UNUSED(p)

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

            CC_FuseFS::Instance()->ccLib->run(cp_c,"sync", p);

            // renew sync filelist in an instance of MSCloudProvider
            CC_FuseFS::Instance()->providerObject->syncFileList.clear();
            CC_FuseFS::Instance()->providerObject->syncFileList = cp_c->syncFileList;

            CC_FuseFS::updateSheduled = false;
            CC_FuseFS::fsBlocked = false;
            delete(cp_c);
        }

    }

}


// ----------------------------------------------------------------

int f_createDirInTemp(const char* name){

    QString fpath = "/tmp/" + tempDirName + name;
    return system(QString("mkdir -p \""+QString(fpath)+"\"").toStdString().c_str());
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
        d.removeRecursively();

    }


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


// ----------------------------------------------------------------

QString getTempName() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString rnd;
    srand (time(NULL));

    for (int i = 0; i < 7; ++i) {
        rnd+= QChar(alphanum[rand() % (sizeof(alphanum) - 1)]);
    }

    return rnd+"-CCFW.temp";
}



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

static int opendir_callback(const char *path, struct fuse_file_info *fi){
    Q_UNUSED(path)
    Q_UNUSED(fi)
    return 0;;
}

// ----------------------------------------------------------------


//static int fgetattr_callback(const char *path, struct stat *stbuf, struct fuse_file_info * fi) {

//    Q_UNUSED(path)
//    Q_UNUSED(fi)
//    Q_UNUSED(stbuf)
//    return -ENOENT;
//}

// ----------------------------------------------------------------

static int getattr_callback(const char *path, struct stat *stbuf) {

    if (QString(path) == QString("/")){

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
                    stbuf->st_size = 40;

                    f_createDirInTemp(path);

                }
                else{

                    stbuf->st_mode = S_IFREG | 0777;
                    stbuf->st_nlink = 1;
                    stbuf->st_size = o.remote.fileSize;
                }

                stbuf->st_uid=getuid();
                stbuf->st_gid=getgid();
                stbuf->st_mtime=o.remote.modifiedDate/1000;
                stbuf->st_blksize = 4096;
                stbuf->st_dev = 37;

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
                        return -ENOENT;
                }

                return 0;
            }
        }


        return -ENOENT;

    }
}

// ----------------------------------------------------------------


static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){

    (void) offset;
    (void) fi;
    log("CALLBACK readdir "+QString(path));


    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

     QString m_path;

     if(QString(path) == "/"){

         m_path=QString(path);
     }
     else{
        m_path=QString(path)+"/";
     }


    QHash<QString, MSFSObject> dl= filterListByPath( /*fileList,*/ QString(m_path));

    //log("CALLBACK readdir  COUNT OF DIR ENT IS "+QString::number(dl.size()));

    if(  dl.size() != 0 ){

        QHash<QString, MSFSObject>::iterator i=dl.begin();

        for(;i != dl.end();i++){

            struct stat st;
            memset(&st, 0, sizeof(struct stat));

            if(i.value().remote.objectType == MSRemoteFSObject::Type::folder){

                st.st_mode = S_IFDIR | 0755;
                st.st_nlink = 2;
//                st.st_size = 40; // MAGIC
            }
            else{

                st.st_mode = S_IFREG | 0777;
                st.st_nlink = 1;
                st.st_size = i.value().remote.fileSize;
            }

            st.st_uid=getuid();
            st.st_gid=getgid();
            st.st_mtime=i.value().remote.modifiedDate/1000;
            st.st_blksize = 4096;
            st.st_dev = 37;


            filler(buf,i.value().fileName.toStdString().c_str(),&st,0);

        }

        return 0;
    }
    else{
        struct stat st;
        memset(&st, 0, sizeof(struct stat));
        st.st_mode = S_IFDIR | 0755;
        st.st_nlink = 2;
        st.st_uid=getuid();
        st.st_gid=getgid();
        st.st_blksize = 4096;
        st.st_dev = 37;


        filler(buf, ".", &st, 0);
        filler(buf, "..", &st, 0);
        return 0;
    }

}


//------------------------------------------------------------------------

static int open_callback(const char *path, struct fuse_file_info *fi) {

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));


    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){


        QString fname=QString("/tmp/")+tempDirName+QString(path);

        struct stat buffer;
        if( stat(fname.toStdString().c_str(), &buffer) == 0){//file exists in cache

            fi->fh = open (fname.toStdString().c_str() ,fi->flags);
            if((int)fi->fh == -1){
                //log("CALLBACK open EXISTS - open for writing error");
                return -1;
            }


            return 0;

        }
        else{// file is missing in cache

            QString ptf=fname.mid(0,fname.lastIndexOf("/"));

            system(QString(QString("mkdir -p \"")+ptf+QString("\"")).toStdString().c_str());

            // create zero-size file
            fstream fs;
            fs.open(fname.toStdString().c_str(), ios::out);
            fs.close();

            fi->fh = open (fname.toStdString().c_str() ,fi->flags);
            if((int)fi->fh == -1){
                //log("CALLBACK open MISSING - open for writing error");
                return -1;
            }

            QMap<QString,QVariant> p;

            p.insert("cachePath",QVariant(fname));
            p.insert("filePath",QVariant(QString(path)));

            CC_FuseFS::Instance()->ccLib->runInSeparateThread(CC_FuseFS::Instance()->providerObject,QString("get_content"),p);
        }

        return 0;
    }

  return -1;
}


// ----------------------------------------------------------------

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    //if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    MSFSObject o;
    unsigned int fullSize;

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

    struct stat buffer;
    if(stat(fname.toStdString().c_str(), &buffer) == 0){//file exists in cache


        if((buffer.st_size == 0)&&(o.local.exist == false)){// file don't cached yet

            if(false){
                FILE* f = fopen(fname.toStdString().c_str(),"rb");
                fseek(f,offset,SEEK_SET);
                size_t cnt = fread(buf,1,size,f);
                fclose(f);

                return cnt;
            }
            else{

                for(int i=0;i < WAIT_FOR_FIRST_DATA;i++){
                    sleep(1);
                    stat (fname.toStdString().c_str(), &buffer);
                    if( (buffer.st_size == fullSize) || (buffer.st_size >= (uint)size) ){

                        return read_callback(path,buf,size,offset,fi);
                    }
                }

                return 0;
            }

        }
        else{// fully or partialy cached

            if(fullSize == buffer.st_size){// fully cached

                    ifstream data;
                    data.open(fname.toStdString().c_str(),   ios::in | ios::binary);//ios::out |ios::trunc |

                    data.seekg(static_cast<streamoff>(offset));
                    data.read(buf, static_cast<streamoff>(size));
                    if (data.eof()){ // read may fail if we reach eof

                        data.clear(); // clear possible failbit
                    }


                    return static_cast<int>(data.gcount());

            }
            else{// partialy cached

               // log("CALLBACK read - file exists and partially cached "+string(path));

                uint endPosition=size+offset-1;

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


static int readlink_callback(const char *path, char *buf, size_t size){

    Q_UNUSED(path)
    Q_UNUSED(buf)
    Q_UNUSED(size)
    return 0;
}


static int mknod_callback(const char *path, mode_t mode, dev_t rdev){
    Q_UNUSED (mode)
    Q_UNUSED(rdev)

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


    QString fname = QString("/tmp/")+tempDirName+QString(path);
    QString insname=QString(path);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    if( i != CC_FuseFS::Instance()->providerObject->syncFileList.end()){

        return -errno;
    }

    log("CALLBACK mknod begin"+fname);

    int res = f_mknod(fname);

    if(res == -1){


        QString td = QString("/tmp/")+tempDirName;
        QString ptf = fname.mid(0,fname.lastIndexOf("/"));



        res = f_mkdir(ptf);
        if(res == -1){
            log("CALLBACK mknod create dir failed. ");
            return -errno;
        }
        else{
            res = f_mknod(fname);
            if(res == -1){

                return -errno;
            }
        }


    }


    CC_FuseFS::Instance()->doSheduleUpdate();

    return 0;
}


static int mkdir_callback(const char *path, mode_t mode){

    Q_UNUSED(mode)

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked



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

            CC_FuseFS::Instance()->doSheduleUpdate();

            return 0;
        }
}


static int unlink_callback(const char *path){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked

    QString fname = QString("/tmp/")+tempDirName+QString(path);

    QHash<QString, MSFSObject>::iterator i= CC_FuseFS::Instance()->providerObject->syncFileList.find(QString(path));

    unlink(fname.toStdString().c_str());

    i.value().state = MSFSObject::ObjectState::DeleteLocal;

    CC_FuseFS::Instance()->doSheduleUpdate();

    return 0;
}


static int rmdir_callback(const char *path){

    if(CC_FuseFS::Instance()->fsBlocked) return -1; // block FS when sync process worked


    QString fname = QString("/tmp/")+tempDirName+QString(path);

    int r = f_rmdir(fname);

    CC_FuseFS::Instance()->doSheduleUpdate();

    return r;
}


static int symlink_callback(const char *from, const char *to){

    Q_UNUSED(from)
    Q_UNUSED(to)

        return 0;
}


static int rename_callback(const char *from, const char *to){

    Q_UNUSED(from)
    Q_UNUSED(to)

    return 0;

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



static int link_callback(const char *from, const char *to){

    Q_UNUSED(from)
    Q_UNUSED(to)

        return 0;
}


static int chmod_callback(const char *path, mode_t mode){

    Q_UNUSED(path)
    Q_UNUSED(mode)

        return 0;
}



static int chown_callback(const char *path, uid_t uid, gid_t gid){

    Q_UNUSED(path)
    Q_UNUSED(uid)
    Q_UNUSED(gid)

        return 0;
}



static int statfs_callback(const char *path, struct statvfs *stbuf){

    Q_UNUSED(stbuf)

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



static int access_callback(const char *path, int mask){

    int res;
    QString fname = QString("/tmp/")+tempDirName+QString(path);

    res = access(fname.toStdString().c_str(), mask);
    if (res == -1)
            return -errno;
    return 0;
}


static int release_callback(const char *path, struct fuse_file_info *fi)
{

    (void) path;
    (void) fi;

    return 0;
}



static int fsync_callback(const char *path, int isdatasync,struct fuse_file_info *fi){

        (void) path;
        (void) isdatasync;
        (void) fi;
        return 0;
}
// ----------------------------------------------------------------


static void destroy_callback(void* d){

    Q_UNUSED(d)

    system(QString("rm -rf /tmp/"+tempDirName).toStdString().c_str());

    pthread_cancel(fs.pid);
    pthread_cancel(fs.timer_pid);
    kill(getpid(), SIGTERM);

}


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


    return;
}


void* fuseLoop(void* p){

    Q_UNUSED(p)
    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

    char* a[2];
    a[0] = (char*)"/usr/bin/true";//argv[0];
    a[1] = (char*)CC_FuseFS::mountPath.toLocal8Bit().data(); //argv[4];
//    a[2]="-s";//
//    a[3]="-f";//

    fs.args = FUSE_ARGS_INIT(2, a);

    fs.mountpoint = CC_FuseFS::mountPath.toLocal8Bit().data();//"/tmp/example";
    fs.multithreaded = 1;
    fs.foreground = 1;

    fuse_parse_cmdline(&fs.args, &fs.mountpoint, &fs.multithreaded, &fs.foreground);

    //fuse_unmount(fs.mountpoint, fs.ch);

    fs.ch = fuse_mount(fs.mountpoint, &fs.args);


    if(fs.foreground == 1){

        qStdOut() << "CloudCross FUSE worker started in background..."<<endl;
        fuse_daemonize(fs.foreground);
    }
    else{

        qStdOut() << "CloudCross FUSE worker started..."<<endl;

    }


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
    fuse_loop_mt(fs.fuse/*, args.clone_fd*/);

    fuse_remove_signal_handlers(se);

        fuse_unmount(fs.mountpoint,fs.ch);

        fuse_destroy(fs.fuse);

        //free(opts.mountpoint);
        fuse_opt_free_args(&fs.args);


        return nullptr;
}



// ----------------------------------------------------------------
//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ms/QT/CloudCross/fuse/libfusecc

int main(int argc, char *argv[])
{
    // argv[0]
    // argv[1] = provider code
    // argv[2] = path to credentials
    // argv[3] = mount point


// Providers code
// 0 - Google
// 1 - Dropbox
// 2 - Yandex
// 3 - Mail
// 4 - OneDrive

    if(argc < 4 ){
        qStdOut() << "CloudCross FUSE worker said: bad parameters!!"<<endl;
        return -1;
    }

   QCoreApplication ca(argc, argv);


   pthread_attr_t attr;
   pthread_attr_init(&attr);


   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


    pthread_create(&(fs.timer_pid), NULL, &onSyncTimer, NULL);

    pthread_detach(fs.timer_pid);

//    pthread_join(tid,(void**)&rv);

//    tokenPath = argv[2];  // "/home/ms/QT/CloudCross/build/debug/TEST4"
//    mountPath = argv[3]; // "/tmp/example"

    tempDirName = "ccfw_"+getTempName();//"TESTING_TEMPORARY";//
    cachePath = "/tmp/"+tempDirName;



    //ccLib = new libFuseCC();

    CC_FuseFS::tokenPath = argv[2];  // "/home/ms/QT/CloudCross/build/debug/TEST4"
    CC_FuseFS::mountPath = argv[3]; // "/tmp/example"

    CC_FuseFS::provider = (ProviderType) QString(argv[1]).toInt(); // QString("4").toInt()

    CC_FuseFS::Instance()->ccLib = new libFuseCC();


    CC_FuseFS::Instance()->ccLib->getProviderInstance(CC_FuseFS::provider, (MSCloudProvider**)&(CC_FuseFS::providerObject), CC_FuseFS::tokenPath);
    CC_FuseFS::providerObject->workPath = cachePath;

    CC_FuseFS::Instance()->ccLib->readRemoteFileList(CC_FuseFS::providerObject);



    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

//    char* a[2];
//    a[0] = argv[0];
//    a[1] = (char*)CC_FuseFS::mountPath.toStdString().c_str(); //argv[4];
////    a[2]="-s";//
////    a[3]="-f";//


    fuse_op.getattr = getattr_callback;
    //fuse_op.fgetattr = fgetattr_callback;
    fuse_op.open = open_callback;
    fuse_op.read = read_callback;
    fuse_op.readdir = readdir_callback;
    fuse_op.destroy = destroy_callback;
    fuse_op.write = write_callback;
    fuse_op.ftruncate = ftruncate_callback;
    fuse_op.truncate = truncate_callback;
    fuse_op.release= release_callback;
    fuse_op.fsync = fsync_callback;

    fuse_op.readlink = readlink_callback;
    fuse_op.mknod = mknod_callback;
    fuse_op.mkdir = mkdir_callback;
    fuse_op.unlink = unlink_callback;
    fuse_op.rmdir = rmdir_callback;
    fuse_op.symlink= symlink_callback;
    fuse_op.rename = rename_callback;
    fuse_op.link = link_callback;
    fuse_op.chmod = chmod_callback;
    fuse_op.chown = chown_callback;
    fuse_op.statfs = statfs_callback;

    fuse_op.access = access_callback;
    fuse_op.opendir = opendir_callback;





    mkdir(QString("/tmp/"+tempDirName).toStdString().c_str(),0777);


    int* pp;
    pthread_attr_t f_attr;
    pthread_attr_init(&f_attr);


    pthread_attr_setdetachstate(&f_attr, PTHREAD_CREATE_DETACHED);


     pthread_create(&(fs.pid), NULL, &fuseLoop, NULL);

     //rc=pthread_detach(fs.pid);
     pthread_join(fs.pid,(void**)&pp);


//    fuse_main(2, a, &fuse_op, NULL);

    return 0;;//ca.exec();
}



