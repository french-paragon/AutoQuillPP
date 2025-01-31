#include "documentitem.h"

DocumentItem::DocumentItem(Type type, QObject *parent) :
    QObject(parent),
    _type(type)
{

}
