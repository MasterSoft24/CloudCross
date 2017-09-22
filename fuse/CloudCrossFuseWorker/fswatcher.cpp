//#include <sys/inotify.h>
//#include <vector>
//#include <poll.h>

//void log(string mes);


//#define EVENT_SIZE  ( sizeof (struct inotify_event) )
//#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

//typedef struct _fswatcher_param{

//    void (*sendCommand) (json command);
//    bool state; // state of thread (true means a thread runned)

//    int wd; // watcher descriptor
//    int watcher;

//    string path; // path to mounted dir
//    string socket;
//    string provider;
//    string credPath; // path to credentials

//}fswatcher_param;


//void* fsWatcher(void* param){
//log("FS WATCHER STARTING");
//    static fswatcher_param* prm= (fswatcher_param*)param;
//    static vector<string> backmem;

//    static char buffer[BUF_LEN];

//    //sleep(3);

//    prm->wd = inotify_init1(IN_NONBLOCK);

//    if(prm->wd < 0){
//        log("INOTIFY: "+string(strerror(errno)));
//        return 0;
//    }


//    string dbgPath="/home/ms/1";
//    log("FS WATCHER ready for watch on "+prm->path);
//    prm->watcher = inotify_add_watch( prm->wd, prm->path.c_str(), IN_CLOSE_WRITE | IN_DELETE | IN_CREATE | IN_OPEN | IN_MOVE | IN_CLOSE_NOWRITE);

//    if(prm->watcher < 0){
//        log("INOTIFY: add watch error "+string(strerror(errno)));
//        close(prm->wd);
//        return 0;
//    }



//    while (true) {

//        int len;

//        struct pollfd pfd = { prm->wd, POLLIN, 0 };

//        int ret = poll(&pfd, 1, 50);

//        if(ret < 0){
//            log("FS WATCHER error reading");
//            close(prm->wd);
//          return 0;
//        }

//        if(ret == 0){

//            //log("FS WATCHER nothing to do");
//            continue;
//        }
//        else{

//            log("FS WATCHER Something read now!!!!!");
//            len = read( prm->wd, &buffer[0], BUF_LEN );

//            if (len < 0) {
//                close(prm->wd);
//                inotify_rm_watch(prm->wd, prm->watcher);
//                log("FS WATCHER error reading");
//              return 0;
//            }
//        }



//log("FS WATCHER was readed");
//        int i=0;

//        while(i < len) {
//            struct inotify_event* event = (struct inotify_event*) &buffer[i];

//            if(event->len) {

//                if( (event->mask & IN_CREATE) ||
//                    (event->mask & IN_OPEN) ){

//                    backmem.push_back(string(&event->name[0]));

//                    json comm;
//                    comm["command"]="set_worker_locked";
//                    comm["params"]["socket"] = prm->socket;
//                    comm["params"]["provider"] = prm->provider;
//                    comm["params"]["path"] = prm->credPath;

//                    prm->sendCommand(comm);

//                    i += EVENT_SIZE + event->len;

//                    continue;
//                }

//                if( (event->mask & IN_DELETE) ||
//                    (event->mask & IN_CLOSE_WRITE) ||
//                    (event->mask & IN_MOVED_TO)||
//                    (event->mask & IN_MOVED_FROM)){

//                    vector<string>::iterator b = backmem.begin();
//                    for(;b != backmem.end();b++){

//                        string ename = string(&event->name[0]);
//                        if(*b == ename){
//                            backmem.erase(b);

//                            json comm;
//                            comm["command"]="set_worker_unlock";
//                            comm["params"]["socket"] = prm->socket;
//                            comm["params"]["provider"] = prm->provider;
//                            comm["params"]["path"] = prm->credPath;

//                            prm->sendCommand(comm);

////                            json comm2;
////                            comm2["command"]="shedule_update";
////                            comm2["params"]["time"] = to_string(time(NULL)); // UNIX timestamp in sec
////                            comm2["params"]["socket"] = prm->socket;
////                            comm2["params"]["provider"] = prm->provider;
////                            comm2["params"]["path"] = prm->credPath;

////                            prm->sendCommand(comm2);

//                            b = backmem.begin();
//                            continue;
//                            //break;
//                        }
//                    }

//                    json comm;
//                    comm["command"]="shedule_update";
//                    comm["params"]["time"] = to_string(time(NULL)); // UNIX timestamp in sec
//                    comm["params"]["socket"] = prm->socket;
//                    comm["params"]["provider"] = prm->provider;
//                    comm["params"]["path"] = prm->credPath;

//                    prm->sendCommand(comm);

//                }
//            }

//            if( (event->mask & IN_CLOSE_NOWRITE)){

//                vector<string>::iterator b = backmem.begin();
//                for(;b != backmem.end();b++){

//                    string ename = string(&event->name[0]);
//                    if(*b == ename){
//                        backmem.erase(b);

//                        json comm;
//                        comm["command"]="set_worker_unlock";
//                        comm["params"]["socket"] = prm->socket;
//                        comm["params"]["provider"] = prm->provider;
//                        comm["params"]["path"] = prm->credPath;

//                        prm->sendCommand(comm);
//                        b = backmem.begin();
//                        continue;
//                        //break;
//                    }
//                }

//            }

//            i += EVENT_SIZE + event->len;
//        }

//    }
//}


