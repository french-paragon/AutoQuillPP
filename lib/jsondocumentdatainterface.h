#ifndef JSONDOCUMENTDATAINTERFACE_H
#define JSONDOCUMENTDATAINTERFACE_H

#include "./documentdatainterface.h"

#include <QJsonObject>

class JsonDocumentDataInterface : public DocumentDataInterface
{
public:
    JsonDocumentDataInterface(QJsonObject const& data, QObject* parent = nullptr);

    virtual DocumentValue getValue(QString const& key);

protected:

    QJsonObject _data;
};

#endif // JSONDOCUMENTDATAINTERFACE_H
