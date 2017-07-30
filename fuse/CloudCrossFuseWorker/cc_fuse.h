#ifndef CC_FUSE_H
#define CC_FUSE_H


#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS  64


#include <QObject>
//#include <QCoreApplication>

#include <fuse.h>
//#include "fuse/fuse_lowlevel.h"

#include "cc_fusefs.h"

class CC_Fuse : public QObject
{
    Q_OBJECT
public:
    explicit CC_Fuse(QObject *parent = 0);

    void start();


public:

    struct   fuse_operations fuse_op;



signals:

public slots:

    void run();
};

#endif // CC_FUSE_H
