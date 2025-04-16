#include "jsondocumentdatainterface.h"

#include <QJsonArray>

namespace AutoQuill {

JsonDocumentDataInterface::JsonDocumentDataInterface(const QJsonObject &data, QObject *parent) :
    DocumentDataInterface(parent),
    _data(data)
{

}

DocumentValue getValueFromValue(QJsonValue const& val);
DocumentValue getValueFromIndex(QJsonArray const& array, int idx);
DocumentValue getValueFromKey(QJsonObject const& obj, QString const& key);

DocumentValue getValueFromValue(QJsonValue const& val) {

    if (val.isObject()) {
        QJsonObject obj = val.toObject();

        return DocumentValue([obj] (QString const& key) -> DocumentValue {
            return getValueFromKey(obj, key);
        });
    }

    if (val.isArray()) {
        QJsonArray array = val.toArray();
        int size = array.size();

        return DocumentValue([array] (int idx) -> DocumentValue {
            return getValueFromIndex(array, idx);
        }, size);
    }

    return DocumentValue([val] () -> QVariant {
        return val.toVariant();
    });

}

DocumentValue getValueFromIndex(QJsonArray const& array, int pidx) {

    int idx = pidx;

    if (pidx < 0) {
        idx = array.size() - pidx;
    }

    if (array.size() <= idx or idx < 0) {
        return DocumentValue();
    }

    QJsonValue val = array[idx];

    return getValueFromValue(val);

}

DocumentValue getValueFromKey(QJsonObject const& obj, QString const& key) {

    if (!obj.contains(key)) {
        return DocumentValue();
    }

    QJsonValue val = obj.value(key);

    return getValueFromValue(val);
}

DocumentValue JsonDocumentDataInterface::getValue(QString const& key) const {

    return getValueFromKey(_data, key);

}

} // namespace AutoQuill
