#include <iostream>

#include <stdio.h>

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS  64
#define DAEMON_MSG_END "\n<---CCFD_MESSAGE_END-->"

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


using namespace std;
using json = nlohmann::json;

static const char *filepath = "/file";
static const char *filename = "file";
static const char *filecontent = "I'm the content of the only file available there\n";

void log(string mes);


// ========= GLOBALS ==========

static int sock_descr;


// ----------------------------------------------------------------

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (strcmp(path, filepath) == 0) {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(filecontent);
    return 0;
  }

  return -ENOENT;
}

// ----------------------------------------------------------------


static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  filler(buf, filename, NULL, 0);

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}


// ----------------------------------------------------------------

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

  if (strcmp(path, filepath) == 0) {
    size_t len = strlen(filecontent);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  return -ENOENT;
}

// ----------------------------------------------------------------



static struct fuse_operations fuse_op;

class ddd{
    int y;
};



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

        log("read portion "+string(data.begin(), data.end()));

        data.resize(oldSize + bytesRead);

        string t=string(data.begin(), data.end());

        if(t.find(DAEMON_MSG_END) != string::npos){
            return t.substr(0,t.find(DAEMON_MSG_END));
        }

    } while (bytesRead > 0);


    return string (data.begin(), data.end());;
}

// ----------------------------------------------------------------

void sendCommand_sync(json command){

//    char buf[1000];

//    snprintf(&buf[0], 1000, "%s^%s^%s^%s", command.c_str(),providerCode.c_str(),path.toStdString().c_str(),addParams.c_str());
//    write(sock_descr,&buf[0],1000);

//    int sz= read(sock_descr,&buf[0],100);


    string j=command.dump();

    log("command to send is - "+j);

    int wsz=write(sock_descr,j.c_str(),j.size());
    if(wsz == -1){
        log("command sending error");
        return;
    }

    fsync(sock_descr);
    log("command was sended");

    string out= readReplyFromCommandServer();

    log("reply was received");

    //cout << out.c_str();
    log(out);
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

    log(std::string(argv[1]));


    if(!connectToCommandServer(std::string(argv[1]))){//std::string(argv[1])
        return 1;
    }


//    std::map<string,ddd*> first;

    fuse_op.getattr = getattr_callback;
    fuse_op.open = open_callback;
    fuse_op.read = read_callback;
    fuse_op.readdir = readdir_callback;


    json comm;
    comm["command"]="get_file_list";
    comm["params"]["socket"]=argv[2];
    comm["params"]["provider"]=argv[2];
    comm["params"]["path"]=argv[3];

    sendCommand_sync(comm);


    return fuse_main(2, a, &fuse_op, NULL);
}



