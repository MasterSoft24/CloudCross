#include <QCoreApplication>

#include "mshttprequest.h"
#include "qmultibuffer.h"

#include <QDataStream>
#include <QProcess>

#define USE_EXECUTOR 1


static void print_cookies(CURL *curl)
{
  CURLcode res;
  struct curl_slist *cookies;
  struct curl_slist *nc;
  int i;

  printf("Cookies, curl knows:\n");
  res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
  if(res != CURLE_OK) {
    fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
            curl_easy_strerror(res));
    exit(1);
  }
  nc = cookies;
  i = 1;
  while(nc) {
    printf("[%d]: %s\n", i, nc->data);
    nc = nc->next;
    i++;
  }
  if(i == 1) {
    printf("(none)\n");
  }
  curl_slist_free_all(cookies);
}


//****************************************************************************

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString host = "https://httpbin.org";
    MSHttpRequest* req;/* = new MSHttpRequest(0);*/


    for(int i=0;i < 10 ;i++){

    qInfo() << "===============      GET     ========================="<<endl;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple GET with 2 params"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("get");
    req->setRequestUrl(host+"/get");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

#ifdef USE_EXECUTOR
    req->exec();
#else
    req->get();
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple GET must return 1024 random bytes as response"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("get");
    req->setRequestUrl(host+"/bytes/1024");

#ifdef USE_EXECUTOR
    req->exec();
#else
    req->get();
#endif

    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple GET must write file with random bytes (100K max)"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("get");
    req->setRequestUrl(host+"/bytes/999999999");

    req->setOutputFile("./mshttprequest-test-get.txt");

#ifdef USE_EXECUTOR
    req->exec();
#else
    req->get();
#endif

    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;

    qInfo() << "===============      POST    ========================="<<endl;

    qInfo() << "======================================================"<<endl;

    qInfo() << " Simple POST with 2 params"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("post");
    req->setRequestUrl(host+"/post");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

#ifdef USE_EXECUTOR
    req->exec();
#else
    req->post("");
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple POST miltipart in QByteArray (for small data)"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("post");
    req->setRequestUrl(host+"/post");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));


    QString bound="msrequest-bound-3467";
    QByteArray mpd;


    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("multipart/form-data; boundary=----")+QString(bound).toLocal8Bit());


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");



    mpd.append("------"+bound+"\r\n");
    mpd.append(QString(QStringLiteral("Content-Disposition: form-data; name=\"file\"; filename=\"")+"tested_file.txt"+QStringLiteral("\"\r\n")));
    mpd.append(QString(QStringLiteral("Content-Type: application/octet-stream\r\n\r\n")).toLocal8Bit());
    mpd.append("this is test file content");
    mpd.append(QString(QStringLiteral("\r\n------")+bound+QStringLiteral("--\r\n")).toLocal8Bit());

    //req->addHeader(QStringLiteral("Content-Length"),QString::number(mpd.length()).toLocal8Bit());

#ifdef USE_EXECUTOR
    req->setInputDataStream(mpd);
    req->exec();
#else
    req->post(mpd);
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple POST with simple payload "<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("post");
    req->setRequestUrl(host+"/post");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json; charset=UTF-8"))  ;


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

    QByteArray payload;

    payload.append("{\"t_val\":\"This is payload for POST request\"}");

    req->addHeader(QStringLiteral("Content-Length"),QString::number(payload.length()).toLocal8Bit());

#ifdef USE_EXECUTOR
    req->setInputDataStream(payload);
    req->exec();
#else
    req->post(payload);
#endif

    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple POST with payload from file  (upload for big data)"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("post");
    req->setRequestUrl(host+"/post");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json; charset=UTF-8"))  ;


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

//    QByteArray payload;

//    payload.append("{\"t_val\":\"This is payload for POST request\"}");

    QFile file("./json.example.txt");

    file.open(QIODevice::ReadOnly);

    req->addHeader(QStringLiteral("Content-Length"),QString::number(file.size()).toLocal8Bit());

#ifdef USE_EXECUTOR
    req->setInputDataStream(&file);
    req->exec();
#else
    req->post(&file);
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    file.close();
    qInfo() << "======================================================"<<endl;
    qInfo() << " Simple POST miltipart in QMultiBuffer  (for big data)"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("post");
    req->setRequestUrl(host+"/post");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("multipart/form-data; boundary=----")+QString(bound).toLocal8Bit());

    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

    mpd.clear();

    mpd.append("------"+bound+"\r\n");
    mpd.append(QString(QStringLiteral("Content-Disposition: form-data; name=\"file\"; filename=\"")+"tested_file.txt"+QStringLiteral("\"\r\n")));
    mpd.append(QString(QStringLiteral("Content-Type: application/json;  charset=UTF-8 \r\n\r\n")).toLocal8Bit());

    QByteArray mpd2;
    mpd2.append(QString(QStringLiteral("\r\n------")+bound+QStringLiteral("--\r\n")).toLocal8Bit());

    QMultiBuffer mb;
    mb.append(&mpd);

    QFile file2("./json.example.txt");

    file2.open(QIODevice::ReadOnly);
    mb.append(&file2);
    mb.append(&mpd2);

    req->addHeader(QStringLiteral("Content-Length"),QString::number(mb.size()).toLocal8Bit());

//    mb.open(QIODevice::ReadOnly);

#ifdef USE_EXECUTOR
    req->setInputDataStream(&mb);
    req->exec();
#else
    req->post(&mb);
#endif

    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    file2.close();
    qInfo() << "======================================================"<<endl;
    qInfo() << "===============      PUT     ========================="<<endl;

    qInfo() << "======================================================"<<endl;

    qInfo() << " Simple PUT with 2 params and small payload"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("put");
    req->setRequestUrl(host+"/put");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json; charset=UTF-8"))  ;


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");

    QByteArray payload2;

    payload2.append("{\"t_val\":\"This is payload for PUT request\"}");

    req->addHeader(QStringLiteral("Content-Length"),QString::number(payload2.length()).toLocal8Bit());

#ifdef USE_EXECUTOR
    req->setInputDataStream(payload2);
    req->exec();
#else
    req->put(payload2);
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;

    qInfo() << " Simple PUT with 2 params and payload from file"<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("put");
    req->setRequestUrl(host+"/put");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json; charset=UTF-8"))  ;


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");


    QFile file3("./json.example.txt");

    req->addHeader(QStringLiteral("Content-Length"),QString::number(file3.size()).toLocal8Bit());

#ifdef USE_EXECUTOR
    req->setInputDataStream(&file3);
    req->exec();
#else
    req->put(&file3);
#endif


    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;

    qInfo() << " Simple DELETE with 2 params "<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("delete");
    req->setRequestUrl(host+"/delete");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    //req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json; charset=UTF-8"))  ;


    req->addQueryItem("q_par1","1");
    req->addQueryItem("q_par2","2");


//    QFile file3("./json.example.txt");

//    req->addHeader(QStringLiteral("Content-Length"),QString::number(file3.size()).toLocal8Bit());

//    req->put(&file3);

#ifdef USE_EXECUTOR
    req->exec();
#else
    req->deleteResource();
#endif

    qInfo() << req->replyText<<endl;

    delete(req);
    req = nullptr;
    qInfo() << "======================================================"<<endl;


    qInfo() << " Remote set cookie test "<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("get");
    req->setRequestUrl(host+"/cookies/set?cookie_n1=cookie_value1&cookie_n2=cookie_value2");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->MSsetCookieJar(new MSNetworkCookieJar(req));


#ifdef USE_EXECUTOR
    req->exec();
#else
    req->get();
#endif


    print_cookies(req->cUrlObject->_curl);

    qInfo() << req->replyText<<endl;

    //delete(req);
    MSHttpRequest* req1 = req;
    req = nullptr;
    qInfo() << "======================================================"<<endl;


    qInfo() << " Remote set cookie test2 (with previously setted cookies) "<<endl;

    req = new MSHttpRequest(0);
    req->setMethod("get");
    req->setRequestUrl(host+"/cookies/set?add_cookie_n1=add_cookie_value1&add_cookie_n2=add_cookie_value2");

    req->addHeader(QStringLiteral("My-Testing_header"),QStringLiteral("test_header/content"));
    req->addHeader(QStringLiteral("My-Testing_header-n2"),QStringLiteral("test_header/content-n2"));

    req->MSsetCookieJar(req1->getCookieJar());


#ifdef USE_EXECUTOR
    req->exec();
#else
    req->get();
#endif


    print_cookies(req->cUrlObject->_curl);

    qInfo() << req->replyText<<endl;

    delete(req);
    delete(req1);
    req = nullptr;
    qInfo() << "======================================================"<<endl;


    }





    return 0;//a.exec();
}
