#include <iostream>
#include <fstream>

#include <stdio.h>

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS  64
#define DAEMON_MSG_END "\n<---CCFD_MESSAGE_END-->"

#define WAIT_FOR_FIRST_DATA 5
#define WAIT_FOR_NEXT_DATA 4

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

#include "json.hpp"

#include "ccfw.h"


using namespace std;
using json = nlohmann::json;

static const char *filepath = "/file";
static const char *filename = "file";
static const char *filecontent = "I'm the content of the only file available there\n";

// forward declarations
void log(string mes);
string sendCommand_sync(json command);
void sendCommand(json command);


// ========= GLOBALS ==========

static struct fuse_operations fuse_op;

static int sock_descr;

std::map<std::string,MSFSObject> fileList;

static string tempDirName;

static string workSocket;
static string workProvider;
static string workPath;



// ----------------------------------------------------------------

int readFileContent(char* path,char* buf,int size, int offset){



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

    return rnd+"-temp";
}




// ----------------------------------------------------------------

map<string, MSFSObject> filterListByPath(map<string, MSFSObject> src, string path){

    map<string, MSFSObject>::iterator i=src.begin();
    map<string,MSFSObject> out;

    for(;i != src.end(); i++){

        MSFSObject o=i->second;
        if(o.path == path){
            out.insert(std::make_pair(i->first,i->second));
        }
    }

    return out;

}



// ----------------------------------------------------------------

static int getattr_callback(const char *path, struct stat *stbuf) {

    if (strcmp(path, "/") == 0){
              stbuf->st_mode = S_IFDIR | 0755;
              stbuf->st_nlink = 1;
              return 0;
    }
    else{

            map<string, MSFSObject>::iterator i= fileList.find(string(path));

                if(i != fileList.end()){

                    //std::pair<string,MSFSObject> o=*i;
                    MSFSObject o= i->second;

                    if(o.remote.objectType == MSRemoteFSObject::Type::folder){

                        stbuf->st_mode = S_IFDIR | 0755;
                    }
                    else{

                        stbuf->st_mode = S_IFREG | 0777;
                        stbuf->st_nlink = 1;
                        stbuf->st_size = o.remote.fileSize;
                    }

                    stbuf->st_uid=1000;
                    stbuf->st_gid=1000;
                    stbuf->st_mtime=o.remote.modifiedDate/1000;

                    return 0;
                }

              return 0;
    }

    return -ENOENT;
}

// ----------------------------------------------------------------


static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){

    (void) offset;
    (void) fi;

     //qDebug()<<" readdir callback"<<endl;

     string m_path;

     if(strcmp(path, "/") == 0){

         m_path=string(path);
     }
     else{
        m_path=string(path)+"/";
     }

    map<string, MSFSObject> dl= filterListByPath( fileList, string(m_path));

    if(dl.size()>0){

        map<string, MSFSObject>::iterator i=dl.begin();

        for(;i != dl.end();i++){

            struct stat st;
            memset(&st, 0, sizeof(struct stat));

            if(i->second.remote.objectType == MSRemoteFSObject::Type::folder){

                st.st_mode = S_IFDIR | 0755;
                st.st_nlink = 1;
            }
            else{

                st.st_mode = S_IFREG | 0777;
                st.st_nlink = 1;
                st.st_size = i->second.remote.fileSize;
            }

            st.st_uid=1000;
            st.st_gid=1000;
            st.st_mtime=i->second.remote.modifiedDate/1000;


            filler(buf,i->second.fileName.c_str(),&st,0);

        }


        return 0;
    }

    return -1;


}

static int open_callback(const char *path, struct fuse_file_info *fi) {

    map<string, MSFSObject>::iterator i= fileList.find(string(path));

    if(i != fileList.end()){

        string fname=string("/tmp/")+tempDirName+string(path);

        struct stat buffer;
        if(stat (fname.c_str(), &buffer) == 0){//file exists

            i->second.refCount++;

        }
        else{// file is missing

            string ptf=fname.substr(0,fname.find_last_of("/"));

            system(string(string("mkdir -p \"")+ptf+string("\"")).c_str());

            fstream fs;
            fs.open(fname.c_str(), ios::out);
            fs.close();

            json comm;
            comm["command"]="get_content";
            comm["params"]["socket"]=workSocket;
            comm["params"]["provider"]=workProvider;
            comm["params"]["path"]=workPath;
            comm["params"]["cachePath"]=fname;
            comm["params"]["filePath"]=string(path);

            sendCommand(comm) ;

            i->second.refCount=0;
        }

        return 0;
    }

  return -1;
}


// ----------------------------------------------------------------

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    log("read "+to_string(size)+" "+to_string(offset));

    map<string, MSFSObject>::iterator i= fileList.find(string(path));

    int fullSize = i->second.remote.fileSize;

    string fname = string("/tmp/")+tempDirName+string(path);

    struct stat buffer;
    if(stat (fname.c_str(), &buffer) == 0){//file exists

        if(buffer.st_size == 0){// file don't cached yet

            for(int i=0;i < WAIT_FOR_FIRST_DATA;i++){
                sleep(1);
                stat (fname.c_str(), &buffer);
                if( (buffer.st_size == fullSize) || (buffer.st_size == size) ){

                    return read_callback(path,buf,size,offset,fi);
                }
            }

            return 0;

        }
        else{// fully or partialy cached

            if(fullSize == buffer.st_size){// fully cached


                FILE* f = fopen(fname.c_str(),"rb");
                fseek(f,offset,SEEK_SET);
                size_t cnt = fread(buf,1,size,f);
                fclose(f);

                return cnt;
            }
            else{// partialy cached

                int endPosition=size+offset-1;

                if(endPosition > fullSize){
                    endPosition = fullSize;
                }

                for(int i=0;i < WAIT_FOR_NEXT_DATA;i++){
                    sleep(1);
                    stat (fname.c_str(), &buffer);
                    if( (endPosition <= buffer.st_size) ){

                        FILE* f = fopen(fname.c_str(),"rb");
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

        return open_callback(path,fi);
    }


return -ENOENT;
}

// ----------------------------------------------------------------


void destroy_callback(void* d){

    system(string("rm -rf /tmp/"+tempDirName).c_str());

    // need send command for destroy socket and objects
}


// ----------------------------------------------------------------

bool connectToCommandServer(const string &socketName){

    struct sockaddr_un name;
    char buf[1000];
    int iMode=1;

    sock_descr=-1;

    memset(&name, '0', sizeof(name));

    name.sun_family = AF_UNIX;

    snprintf(name.sun_path, 200, "/tmp/%s", socketName.c_str());

    int s=socket(PF_UNIX,SOCK_STREAM,0);

    if(s== -1){
        return false;
    }

    int r=connect(s, (struct sockaddr *)&name, sizeof(struct sockaddr_un));

    if(r<0){
        perror ("connect");
        return false;
    }

    int sz= read(s,&buf[0],100);

    if(sz >0){
        string reply=&buf[0];

        if(reply.find("HELLO") != string::npos){

            //ioctl(s, FIONBIO, &iMode);
            fcntl(s,F_SETFD,O_NONBLOCK);
            sock_descr=s; // storing socket descriptor globaly
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------

string readReplyFromCommandServer(){

    typedef std::vector<char> replyBuffer;
    replyBuffer data;
    int bytesRead ;

    log("ready for receiving data from daemon");

    do
    {
        static const int bufferSize = 1024;

        const size_t oldSize = data.size();
        data.resize(data.size() + bufferSize);

        bytesRead=recv(sock_descr,&data[oldSize],bufferSize,0);//bufferSize

        //log("read portion "+string(data.begin(), data.end()));

        data.resize(oldSize + bytesRead);

        string t=string(data.begin(), data.end());

        if(t.find(DAEMON_MSG_END) != string::npos){
            return t.substr(0,t.find(DAEMON_MSG_END));
        }

    } while (bytesRead > 0);


    return string (data.begin(), data.end());;
}

// ----------------------------------------------------------------


string sendCommand_sync(json command){

    string j=command.dump();

    log("command to send is - "+j);

    int wsz=write(sock_descr,j.c_str(),j.size());
    if(wsz == -1){
        log("command sending error");
        return "";
    }

    fsync(sock_descr);
    log("command was sended");

    string out= readReplyFromCommandServer();

    log("reply was received");

    //cout << out.c_str();
    //log(out);
    return out;
}


// ----------------------------------------------------------------

void sendCommand(json command){

    string j=command.dump();

    log("command to send is - "+j);

    int wsz=write(sock_descr,j.c_str(),j.size());
    if(wsz == -1){
        log("command sending error");
        return;
    }

    fsync(sock_descr);
    log("command was sended");

//    string out= readReplyFromCommandServer();

//    log("reply was received");

//    //cout << out.c_str();
//    //log(out);
//    return out;
}



// ----------------------------------------------------------------

void log(string mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        mes+="\n";
        fputs(mes.c_str(),lf);
        fclose(lf);
    }

//    string ns="echo "+mes+" >> /tmp/ccfw.log ";
//    system(ns.c_str());
    return;
}

// ----------------------------------------------------------------


int main(int argc, char *argv[])
{
    // argv[0]
    // argv[1] = socket name for communicate with command server
    // argv[2] = provider code
    // argv[3] = path to credentials
    // argv[4] = mount point


    // rebuild arguments list
    char* a[2];
    a[0]=argv[0];
    a[1]="/tmp/example";

//    log(std::string(argv[1]));


    if(!connectToCommandServer(std::string(argv[1]))){//std::string(argv[1])
        return 1;
    }


    fuse_op.getattr = getattr_callback;
    fuse_op.open = open_callback;
    fuse_op.read = read_callback;
    fuse_op.readdir = readdir_callback;
    fuse_op.destroy = destroy_callback;

    workSocket=argv[1];
    workProvider=argv[2];
    workPath=argv[3];

    json comm;
    comm["command"]="get_file_list";
    comm["params"]["socket"]=argv[1];
    comm["params"]["provider"]=argv[2];
    comm["params"]["path"]=argv[3];

    json jsonFileList= json::parse( sendCommand_sync(comm) );

//            json jsonFileList;     // test
//            std::ifstream i("tst.json");

//            i >> jsonFileList;
//            //============================

    for(json::iterator i=jsonFileList.begin();i != jsonFileList.end();i++){

        MSFSObject obj;
        json jo=i.value();
        obj.state               = jo["state"];
        obj.path                = jo["path"];
        obj.fileName            = jo["fileName"];
        obj.refCount            = -1;

        obj.remote.data         = jo["remote"]["data"];
        obj.remote.exist        = true;
        obj.remote.fileSize     = jo["remote"]["fileSize"];
        obj.remote.md5Hash      = jo["remote"]["md5Hash"];
        obj.remote.modifiedDate = jo["remote"]["modifiedDate"];
        obj.remote.objectType   = jo["remote"]["objectType"];


        fileList.insert(std::make_pair(i.key(),obj));

        //log(i.key());
        //std::cout << i.key();
    }

    tempDirName="TESTING_TEMPORARY";//string(argv[1])+string("_")+getTempName();

    mkdir(string("/tmp/"+tempDirName).c_str(),0755);

    return fuse_main(2, a, &fuse_op, NULL);
}



