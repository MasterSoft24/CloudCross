#include "cc_fuse.h"

CC_Fuse::CC_Fuse(QObject *parent) : QObject(parent){

}

void CC_Fuse::start(){

    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

    CC_FuseFS::Instance()->mountPath="/tmp/example";

    char* a[2];
    a[0] = "/home/ms/QT/CloudCross/fuse/build-CloudCrossFuseWorker-Desktop-Debug/CloudCrossFuseWorker";//argv[0];
    a[1] = (char*)CC_FuseFS::Instance()->mountPath.toStdString().c_str(); //argv[4];
//    a[2]="-f";//
//    a[3]="-s";//
     int argc=2;

//     struct fuse_args args = FUSE_ARGS_INIT(argc, a);

//         int res,multithreaded =0,foreground =0;

//         // parses command line options (mountpoint, -s, foreground(which is ignored) and -d as well as other fuse specific parameters)
//         res = fuse_parse_cmdline(&args, &a[1], &multithreaded, &foreground);
//         if (res == -1) {

//             return;
//         }

//          fuse_chan* ch =fuse_mount(a[1], &args);

//          if(!ch){
//              return;
//          }

//          fuse *fuse = fuse_new(ch, &args, &this->fuse_op, sizeof(fuse_operations), NULL);

//    int qw=fuse_main(2, a, &(this->fuse_op), NULL);
//    int y=86;

}

void CC_Fuse::run(){

    // rebuild arguments list for fuse
    // -s single-thread
    // -d debug
    // -f foreground
    // -o [no]rellinks

//    char* aa[3];
//    aa[0] = "/home/ms/QT/CloudCross/fuse/build-CloudCrossFuseWorker-Desktop-Debug/CloudCrossFuseWorker";//argv[0];
//    aa[1] = (char*)CC_FuseFS::Instance()->mountPath.toStdString().c_str(); //argv[4];
//    aa[2] = NULL;
////    a[2]="-f";//
////    a[3]="-s";//
//     int argc=3;
//    QCoreApplication* ca = new QCoreApplication(argc,aa);

    CC_FuseFS::Instance()->mountPath="/tmp/example";

    char* a[3];
    a[0] = "/home/ms/QT/CloudCross/fuse/build-CloudCrossFuseWorker-Desktop-Debug/CloudCrossFuseWorker";//argv[0];
    a[1] = (char*)CC_FuseFS::Instance()->mountPath.toStdString().c_str(); //argv[4];
    a[2] = NULL;
//    a[2]="-f";//
//    a[3]="-s";//
     //int argc=2;

     int qw=fuse_main(2, a, &(this->fuse_op), NULL);



}









