#include "documentitem.h"
#include "documenttemplate.h"

#include <QIcon>

#include <QJsonObject>
#include <QJsonArray>

#include <QMetaObject>
#include <QMetaProperty>

DocumentItem::DocumentItem(Type type, QObject *parent) :
    QObject(parent),
	_type(type),
	_direction(Top2Bottom)
{
	if (_type == Page) {
		_initial_width = 595;
		_initial_height = 842;
	}

	if (_type == Frame or _type == Text or _type == Image) {
		_initial_width = 100;
		_initial_height = 60;
		_max_width = 100;
		_max_height = 60;
	}
}

QIcon DocumentItem::iconForType(Type const& type) {

	switch (type) {
	case Frame:
		return QIcon(":/icons/frame.svg");
	case Text:
		return QIcon(":/icons/text.svg");
	case Image:
		return QIcon(":/icons/image.svg");
	case Page:
		return QIcon(":/icons/page.svg");
	case Condition:
		return QIcon(":/icons/condition.svg");
	case List:
		return QIcon(":/icons/list.svg");
	case Loop:
		return QIcon(":/icons/loop.svg");
	case Plugin:
		return QIcon(":/icons/plugin.svg");
	case Invalid:
		return QIcon();
	}

	return QIcon();

}
QString DocumentItem::typeToString(Type const& type) {

	switch (type) {
	case Frame:
		return tr("Frame");
	case Text:
		return tr("Text");
	case Image:
		return tr("Image");
	case Page:
		return tr("Page");
	case Condition:
		return tr("Condition");
	case List:
		return tr("List");
	case Loop:
		return tr("Loop");
	case Plugin:
		return tr("Plugin");
	case Invalid:
		return tr("Invalid");

	}

	return {};
}
DocumentItem::Type DocumentItem::stringToType(QString const& str) {
	QString lower = str.toLower();

	for (Type & t : QList<Type>{Frame, Text, Image, Page, Condition, List, Loop, Plugin, Invalid}) {
		if (lower == typeToString(t).toLower()) {
			return t;
		}
	}

	return Invalid;
}

QList<DocumentItem::Type> DocumentItem::supportedRootTypes() {
	return {Loop, Condition, Page}; //at the root level we can have a page, some conditional pages or a loop of pages
}

int DocumentItem::pageId() {
	QObject* p = parent();

	if (p == nullptr) {
		return -1;
	}

	DocumentTemplate* docTemplate = qobject_cast<DocumentTemplate*>(p);

	if (docTemplate != nullptr) {
		return docTemplate->subitems().indexOf(this);
	}

	DocumentItem* docItem = qobject_cast<DocumentItem*>(p);

	if (docItem != nullptr) {
		return docItem->pageId();
	}

	return -1;
}

QList<DocumentItem::Type> DocumentItem::supportedSubTypes() {

	switch (_type) {

	case Frame:
		return {Frame, Text, Image, Condition, List, Loop, Plugin};

	case Text:
		return {};

	case Image:
		return {};

	case Page:
		return {Frame, Text, Image, Condition, List, Loop, Plugin};

	case Condition:
	case List:
	case Loop: {
		DocumentItem* parent = parentDocumentItem();
		if (parent == nullptr) {
			return supportedRootTypes();
		}
		return parent->supportedSubTypes();
	}

	case Plugin:
		return {};

	case Invalid:
		return {};

	}

	return {};
}


QJsonValue DocumentItem::encapsulateToJson() const {

    QJsonObject obj;

	obj.insert("type", typeToString(_type));

    const QMetaObject* mobj = metaObject();

    for (int i = 0; i < mobj->propertyCount(); i++) {

        if (!mobj->property(i).isStored(this)) {
            continue;
        }

        const char* prop = mobj->property(i).name();

		if (QString(prop) == "type") {
			continue; //reserved for type
        }

		if (QString(prop) == "subitems") {
			continue; //reserved for subblocks
		}

        if (!propertyIsStoredForCurrentType(prop)) {
            continue;
        }

        QVariant val = property(prop);

        if (val.type() == QVariant::Color) {
            QColor col = val.value<QColor>();
            QString str = col.name(QColor::HexArgb);
            obj.insert(prop, str);
        } else {
            obj.insert(prop, QJsonValue::fromVariant(val));
        }
    }

    QJsonArray blocks;

    for (DocumentItem* item : _items) {
        if (item == nullptr) {
            continue;
        }

        blocks.push_back(item->encapsulateToJson());
    }

	obj.insert("subitems", blocks);

    return obj;

}


bool DocumentItem::propertyIsStoredForCurrentType(const char* propName) const {

    QString pName(propName);

    switch(_type) {
    case Invalid:
        return false;
    case Page:
        if (pName == "posX" or
            pName == "posY" or
            pName == "maxWidth" or
            pName == "maxHeight" or
            pName == "fontSize") {
            return false;
        }
        break;
    case Condition:
        if (pName == "posX" or
            pName == "posY" or
            pName == "initialWidth" or
            pName == "initialHeight" or
            pName == "maxWidth" or
            pName == "maxHeight" or
            pName == "borderWidth" or
            pName == "borderColor" or
            pName == "fillColor" or
            pName == "fontSize") {
            return false;
        }
    case Loop:
    case List:
    case Plugin:
    case Image:
    case Frame:
        if (pName == "fontSize") {
            return false;
        }
    default:
        break;
    };

    return true;
}

DocumentItem* DocumentItem::buildFromJson(QJsonValue const& value) {

	if (!value.isObject()) {
		return nullptr;
	}

	QJsonObject obj = value.toObject();

	if (!obj.contains("type")) {
		return nullptr;
	}

	Type type = stringToType(obj.value("type").toString());

	if (type == Type::Invalid) {
		return nullptr;
	}

	DocumentItem* item = new DocumentItem(type);

	const QMetaObject* mobj = item->metaObject();

	for (int i = 0; i < mobj->propertyCount(); i++) {

		if (!mobj->property(i).isStored(item)) {
			continue;
		}

		const char* prop = mobj->property(i).name();

		if (QString(prop) == "type") {
			continue; //reserved for type
		}

		if (QString(prop) == "subitems") {
			continue; //reserved for subblocks
		}

		if (!obj.contains(prop)) {
			continue;
		}

		QJsonValue val = obj.value(prop);

		if (mobj->property(i).type() == QVariant::Color) {
			QColor col = QColor(val.toString());
			item->setProperty(prop, col);
		} else if (mobj->property(i).type() == QVariant::Int or
				   mobj->property(i).type() == QVariant::UInt or
				   mobj->property(i).type() == QVariant::LongLong or
				   mobj->property(i).type() == QVariant::ULongLong) {
			item->setProperty(prop, QVariant(val.toInt()));
		} else if (mobj->property(i).type() == QVariant::Double) {
			item->setProperty(prop, QVariant(val.toDouble()));
		} else {
			item->setProperty(prop, QVariant(val.toString()));
		}
	}

	if (!obj.contains("subitems")) {
		return item;
	}

	QJsonValue subitems = obj.value("subitems");

	if (!subitems.isArray()) {
		return item;
	}

	QJsonArray array = subitems.toArray();

	for (QJsonValue const& val : qAsConst(array)) {

		DocumentItem* subitem = DocumentItem::buildFromJson(val);

		if (subitem == nullptr) {
			continue;
		}

		item->insertSubItem(subitem);
	}

	return item;
}
