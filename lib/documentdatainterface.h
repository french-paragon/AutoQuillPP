#ifndef DOCUMENTDATAINTERFACE_H
#define DOCUMENTDATAINTERFACE_H

#include <QObject>
#include <QVariant>

#include <functional>

namespace AutoQuill {

class DocumentValue
{
public:
    DocumentValue();
	DocumentValue(std::function<DocumentValue(int)> const& arrayReader, int size);
    DocumentValue(std::function<DocumentValue(QString)> const& mapReader);
    DocumentValue(std::function<QVariant()> const& dataReader);

    ~DocumentValue();

	inline explicit operator bool() const {
        return bool(_readerArray) or bool(_readerMap) or bool(_readerData);
    }

	inline bool hasArray() const {
        return bool(_readerArray);
    }

	inline int arraySize() const {
		return _arraySize;
	}

	inline bool hasMap() const {
        return bool(_readerMap);
    }

	inline bool hasData() const {
        return bool(_readerData);
    }

    inline DocumentValue getValue(int index) const {
        if (!_readerArray) {
            return DocumentValue();
        }
        return _readerArray(index);
    }

    inline DocumentValue getValue(QString const& key) const {
        if (!_readerMap) {
            return DocumentValue();
        }
        return _readerMap(key);
    }

    inline QVariant getValue() const {
        if (!_readerData) {
            return QVariant();
        }
        return _readerData();
    }

protected:
    std::function<DocumentValue(int)> _readerArray;
	int _arraySize;
    std::function<DocumentValue(QString)> _readerMap;
    std::function<QVariant()> _readerData;

};

class DocumentDataInterface : public QObject
{
    Q_OBJECT
public:
    DocumentDataInterface(QObject* parent = nullptr);

    virtual DocumentValue getValue(QString const& key) = 0;

};

} // namespace AutoQuill

#endif // DOCUMENTDATAINTERFACE_H
