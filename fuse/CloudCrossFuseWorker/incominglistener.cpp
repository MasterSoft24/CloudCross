
//void log(string mes);

//typedef struct _lst_param{

//    int (*onIncomingCommand) (const char *);
//    int incomingSocket;
//    string socketName;
//    bool state; // state of thread (true means thread runned)

//}lst_param;

//void* incomingListener(void* param){

//    lst_param* prm= (lst_param*)param;

//    struct sockaddr_un name;
//    struct sockaddr_un cli_addr;
//    static char buf[1000];

//    socklen_t clilen;

//    prm->incomingSocket=-1;

//    memset(&name, '0', sizeof(name));

//    name.sun_family = AF_UNIX;

//    snprintf(name.sun_path, 200, "/tmp/%s", prm->socketName.c_str());

//    int s=socket(PF_UNIX,SOCK_STREAM,0);

//    if(s== -1){
//        return NULL;
//    }

//    prm->incomingSocket=s;

//    int r=bind(s, (struct sockaddr *)&name, sizeof(struct sockaddr_un));

//    if(r<0){
//        perror ("connect");
//        return NULL;
//    }

//    prm->state=true;

//log("INCOMING LISTENER");
//    while(true){

//        listen(prm->incomingSocket,1);

//        clilen = sizeof(cli_addr);

//        int newsockfd = accept(prm->incomingSocket,(struct sockaddr *) &cli_addr, &clilen);

//        bzero(buf,1000);
//        int n = read(newsockfd,buf,999);

//        if(prm->onIncomingCommand != NULL){
//            prm->onIncomingCommand(&buf[0]);
//        }

//        close(newsockfd);
//    }

//}
