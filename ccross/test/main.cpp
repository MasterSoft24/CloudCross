#include <QCoreApplication>


#include "common.h"


#include "msoptparser_test.h"



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    MSOptParser_test* optparser=new MSOptParser_test();
    optparser->init();

    optparser->start(1);
    optparser->start(2);
    optparser->start(3);
    optparser->start(4);
    optparser->start(5);
    optparser->start(6);
    optparser->start(7);
    optparser->start(8);
    optparser->start(9);

    return 0;
}
