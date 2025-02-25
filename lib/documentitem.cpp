#include "documentitem.h"

#include <QIcon>

DocumentItem::DocumentItem(Type type, QObject *parent) :
    QObject(parent),
    _type(type)
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
