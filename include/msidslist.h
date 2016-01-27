#ifndef MSIDSLIST_H
#define MSIDSLIST_H

#include <QList>

class MSIDsList
{
public:
    MSIDsList();

    QList<QString> ids_list;

    QList<QString>::iterator ids_iterator;

    void setList(QList<QString> list);
    QString getID();

};

#endif // MSIDSLIST_H
