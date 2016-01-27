
#include "include/qstdout.h"

QTextStream& qStdOut()
{
    static QTextStream ts( stdout );
    return ts ;
}

