#include "documentdatainterface.h"

namespace AutoQuill {

DocumentValue::DocumentValue() :
	_arraySize(0)
{

}
DocumentValue::DocumentValue(std::function<DocumentValue(int)> const& arrayReader, int size)  :
	_readerArray(arrayReader),
	_arraySize(size)
{

}
DocumentValue::DocumentValue(std::function<DocumentValue(QString)> const& mapReader)  :
	_readerMap(mapReader),
	_arraySize(0)
{

}
DocumentValue::DocumentValue(std::function<QVariant()> const& dataReader)  :
	_readerData(dataReader),
	_arraySize(0)
{

}

DocumentValue::~DocumentValue() {

}

DocumentDataInterface::DocumentDataInterface(QObject *parent) :
    QObject(parent)
{

}

} // namespace AutoQuill
