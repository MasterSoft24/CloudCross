#ifndef MSOPTPARSER_TEST_H
#define MSOPTPARSER_TEST_H

#include <QHash>

#include "common.h"
#include "include/msoptparser.h"
#include "include/qstdout.h"

typedef struct _test_result{

    QString text;
    bool result;
    int control;


}test_result;


typedef struct _test_descr{

    QString testName;
    int inpParamNumber;
    int control;
    int resultCount;
    QStringList controlList;

}test_descr;





class MSOptParser_test
{
public:
    MSOptParser_test();

    MSOptParser* parser;
    QList<QStringList> parameters;
    QHash<QString, test_result> results;
    QList<test_descr> tests;

    void init();

    void setParserArgs();

    int start(const int testNumber);
    int test(const QStringList& params);
    void printResults(test_descr dsc);
};

#endif // MSOPTPARSER_TEST_H
