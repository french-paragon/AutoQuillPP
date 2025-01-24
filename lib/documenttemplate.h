#ifndef DOCUMENTTEMPLATE_H
#define DOCUMENTTEMPLATE_H

#include <QObject>

class DocumentTemplate : QObject
{
    Q_OBJECT
public:
    DocumentTemplate(QObject* parent = nullptr);
};

#endif // DOCUMENTTEMPLATE_H
