#ifndef JSONDOCUMENTDATAINTERFACE_H
#define JSONDOCUMENTDATAINTERFACE_H

#include "./documentdatainterface.h"

#include <QJsonObject>

namespace AutoQuill {

class JsonDocumentDataInterface : public DocumentDataInterface
{
public:
    JsonDocumentDataInterface(QJsonObject const& data, QObject* parent = nullptr);

    virtual DocumentValue getValue(QString const& key);

protected:

    QJsonObject _data;
};

} // namespace AutoQuill

#endif // JSONDOCUMENTDATAINTERFACE_H
