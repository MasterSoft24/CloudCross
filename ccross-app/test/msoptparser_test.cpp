#include "msoptparser_test.h"

MSOptParser_test::MSOptParser_test(){

    this->parser=new MSOptParser();
}


void MSOptParser_test::init(){

    parameters.append((QStringList())<<"ccross_tst"); // run without parameters

    parameters.append((QStringList())<<"ccross_tst"<<"--wrong"); // run with once wrong parameter

    parameters.append((QStringList())<< "ccross_tst"  << "-h"); // run with short param

    parameters.append((QStringList())<< "ccross_tst" << "--version" ); // run with long param

    parameters.append((QStringList())<< "ccross_tst" << "-p" << "arg"); // run with short param with 1 argument

    parameters.append((QStringList())<< "ccross_tst" << "--path" << "arg"); // run with long param with 1 argument


    parameters.append((QStringList())<< "ccross_tst" << "--direct-upload" << "arg1" << "arg2"); // run with long param with 2 argument

    parameters.append((QStringList())<< "ccross_tst" << "-p" << "arg" << "--direct-upload" << "arg1" << "arg2");// Run 1 non-block and 1 param with 2 argument

    parameters.append((QStringList())<< "ccross_tst" << "--direct-upload" << "arg1" << "arg2" << "-p" << "arg");// Run 1 param with 2 argument and 1 non-block

    parameters.append((QStringList())<< "ccross_tst" << "" << "");

    parameters.append((QStringList())<< "ccross_tst" << "" << "");

    parameters.append((QStringList())<< "ccross_tst" << "" << "");




    tests.append( test_descr{"all tests"});
    tests.append( test_descr{"Run without params",                          0,1,1});
    tests.append( test_descr{"Run with once wrong parameter",               1,2,1});
    tests.append( test_descr{"Run with short parameter",                    2,3,1});
    tests.append( test_descr{"Run with long parameter",                     3,4,1});
    tests.append( test_descr{"Run with short param with 1 argument",        4,1,2,(QStringList()<<"run without params"<<"run  with short param with 1 argument")});
    tests.append( test_descr{"Run with long param with 1 argument",        5,1,2,(QStringList()<<"run without params"<<"run  with short param with 1 argument")});
    tests.append( test_descr{"Run with long param with 2 argument",        6,6,1});
    tests.append( test_descr{"Run 1 non-block and 1 param with 2 argument",        7,0,2,(QStringList()<<"run  with short param with 1 argument"<<"run  with long param with 2 argument")});
    tests.append( test_descr{"Run 1 param with 2 argument and 1 non-block",        8,6,1,(QStringList()<<"run  with short param with 1 argument"<<"run  with long param with 2 argument")});


    setParserArgs();

}

void MSOptParser_test::setParserArgs(){

    parser->insertOption(1,"--help -h  ");
    parser->insertOption(2,"-v --version");
    parser->insertOption(3,"-p --path 1"); // one parameter required for this option
    parser->insertOption(4,"--direct-upload 2"); // upload file directly to cloud

}







int MSOptParser_test::start(const int testNumber){

    int ret=0;


    switch (testNumber) {
    case 0:
        ret+=this->test(this->parameters.at(0));// run without parameters

        break;
    case 1: {// run without parameters

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 2: {// run with once wrong parameter

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 3: {// run with short parameter

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 4: {// run with long parameter

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 5: {// run  with short param with 1 argument

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 6: {// run  with long param with 1 argument

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 7: {// run  with long param with 2 argument

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 8: {// run  with long param with 2 argument

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }

    case 9: {// run  with long param with 2 argument

        test_descr ct= (tests.at(testNumber));
        ret+=this->test(this->parameters.at(ct.inpParamNumber));
        printResults(ct);
        break;
    }



//    default:
//        break;
    }


}




int MSOptParser_test::test(const QStringList& params){

    test_result res;
    res.result=false;



    parser->parse(params);// parse command line parameters

    int ret;

    while((ret=parser->get())!=-1){// command line pagameters processing loop
        switch(ret){

        case 1:
            res.result=true;
            res.control=3;
            res.text=parser->errorString;
            results.insert("run with short param",res);
            return 0;
            break;

        case 2:
            res.result=true;
            res.control=4;
            res.text=parser->errorString;
            results.insert("run with long param",res);
            return 0;

            break;

        case 3:

            res.result=true;
            res.control=5;
            res.text=parser->errorString;
            results.insert("run  with short param with 1 argument",res);
            break;

        case 4:

            res.result=true;
            res.control=6;
            res.text=parser->errorString;
            results.insert("run  with long param with 2 argument",res);
            return 0;
            break;


        default:
            res.result=true;
            res.control=1;
            results.insert("run without params",res);

            return 0;
            break;
        }


    }

    if(parser->erorrNum!=0){
        res.result=true;
        res.control=2;
        res.text=parser->errorString;
        results.insert("run with once wrong params",res);


        return -1;
    }



}



void MSOptParser_test::printResults(test_descr dsc){

    qStdOut()<<endl;

    if(dsc.resultCount ==1){

        qStdOut()<< dsc.testName<<" ";

        if(dsc.resultCount == results.size()){

            if((dsc.control == results.begin().value().control) && results.begin().value().result){
                qStdOut()<<"   Test OK "<<"<- PASSED"<<endl;
            }
            else{
                qStdOut()<<RED_TEXT<<"   Missmath  test control values or test result "<<"<- NOT PASSED"<<RED_TEXT_END<<endl;
            }
        }
        else{
            qStdOut()<<RED_TEXT<<"   Missmath results count "<<"<- NOT PASSED"<<RED_TEXT_END<<endl;
        }
    }
    else{

        qStdOut()<< dsc.testName<<" ";

        if(dsc.resultCount == results.size()){

            bool tr=true;
            for(int i=0;i< dsc.controlList.size();i++){

                if( results.find( dsc.controlList.at(i)) != results.end() ){

                    if(results.find( dsc.controlList.at(i)).value().result != true){
                        tr=false;
                        break;
                    }
                }
                else{
                    tr=false;
                    break;
                }

            }


            if(tr){
                qStdOut()<<"   Test OK "<<"<- PASSED"<<endl;
            }
            else{
                qStdOut()<<RED_TEXT<<"   Missmath  test control values or test result "<<"<- NOT PASSED"<<RED_TEXT_END<<endl;
            }
        }
        else{
            qStdOut()<<RED_TEXT<<"   Missmath results count "<<"<- NOT PASSED"<<RED_TEXT_END<<endl;
        }

    }



    results.clear();
    delete(parser);
    parser=new MSOptParser();

    setParserArgs();



}


