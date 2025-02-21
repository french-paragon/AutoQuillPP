#include "documenttemplate.h"

#include "documentitem.h"

#include <QIcon>

DocumentTemplate::DocumentTemplate(QObject *parent) :
    QObject(parent)
{

}

bool DocumentTemplate::removeSubItem(int index) {
	if (index < 0 or index >= _items.size()) {
		return false;
	}

	_items.removeAt(index);
	return true;
}

bool DocumentTemplate::moveSubItem(int previousIndex, int newIndex) {
	if (previousIndex < 0 or newIndex < 0 or previousIndex >= _items.size() or newIndex >= _items.size()) {
		return false;
	}

	_items.move(previousIndex, newIndex);
	return true;
}

void DocumentTemplate::insertSubItem(DocumentItem* item, int position) {

	item->setParent(this);

	if (position < 0) {
		_items.insert((_items.size()+1+position)%(_items.size()+1), item);
		return;
	}

	if (position >= _items.size()) {
		_items.insert(_items.size(), item);
		return;
	}

	_items.insert(position, item);
}

DocumentTemplateModel::DocumentTemplateModel(QObject* parent) :
	QAbstractItemModel(parent),
	_root(nullptr)
{

}


QModelIndex DocumentTemplateModel::index(int row, int column, const QModelIndex &parent) const {

	if (parent == QModelIndex()) {
		return createIndex(row, column, _root->subitems()[row]);
	}

	DocumentItem* parentObj = reinterpret_cast<DocumentItem*>(parent.internalPointer());

	return createIndex(row, column, parentObj->subitems()[row]);
}

QModelIndex DocumentTemplateModel::parent(const QModelIndex &index) const {

	DocumentItem* obj = reinterpret_cast<DocumentItem*>(index.internalPointer());
	QObject* parentObj = obj->parent();

	if (parentObj == nullptr) {
		return QModelIndex();
	}

	if (qobject_cast<DocumentTemplate*>(parentObj) != nullptr) {
		return QModelIndex();
	}

	DocumentItem* pItem = qobject_cast<DocumentItem*>(parentObj);

	if (pItem == nullptr) {
		return QModelIndex();
	}

	QObject* parentParentObj = parentObj->parent();

	if (parentParentObj == nullptr) {
		return QModelIndex();
	}

	if (qobject_cast<DocumentTemplate*>(parentParentObj) != nullptr) {
		int row = _root->children().indexOf(pItem);
		return createIndex(row,0,pItem);
	}

	DocumentItem* ppItem = qobject_cast<DocumentItem*>(parentParentObj);

	if (ppItem == nullptr) {
		return QModelIndex();
	}

	int row = ppItem->subitems().indexOf(pItem);

	return createIndex(row,0,pItem);

}

int DocumentTemplateModel::rowCount(const QModelIndex &parent) const {

	if (parent == QModelIndex()) {
		return _root->children().size();
	}

	DocumentItem* obj = reinterpret_cast<DocumentItem*>(parent.internalPointer());

	return obj->subitems().size();

}
int DocumentTemplateModel::columnCount(const QModelIndex &parent) const {
	return 1;
}

QVariant DocumentTemplateModel::data(const QModelIndex &index, int role) const {

	if (index == QModelIndex()) {
		return QVariant();
	}

	DocumentItem* target = reinterpret_cast<DocumentItem*>(index.internalPointer());

	if (target == nullptr) {
		return QVariant();
	}

	switch (role) {
	case ItemRole:
		return QVariant::fromValue(target);
	case Qt::DisplayRole:
		return QString("[%1] ").arg(DocumentItem::typeToString(target->getType())) +  target->objectName();
	case Qt::DecorationRole:
		switch (target->getType()) {
		case DocumentItem::Page:
			return QIcon(":/icons/page.svg");
		case DocumentItem::Frame:
			return QIcon(":/icons/frame.svg");
		case DocumentItem::Text:
			return QIcon(":/icons/text.svg");
		case DocumentItem::Image:
			return QIcon(":/icons/image.svg");
		case DocumentItem::Condition:
			return QIcon(":/icons/condition.svg");
		case DocumentItem::List:
			return QIcon(":/icons/list.svg");
		case DocumentItem::Loop:
			return QIcon(":/icons/loop.svg");
		case DocumentItem::Plugin:
			return QIcon(":/icons/plugin.svg");
		};
		break;
	case Qt::EditRole:
		return target->objectName();
	}

	return QVariant();
}

bool DocumentTemplateModel::setData(const QModelIndex &index, const QVariant &value, int role) {

	if (index == QModelIndex()) {
		return false;
	}

	DocumentItem* target = reinterpret_cast<DocumentItem*>(index.internalPointer());

	if (target == nullptr) {
		return false;
	}

	if (role != Qt::EditRole and role != Qt::DisplayRole) {
		return false;
	}

	target->setObjectName(value.toString());

	return true;

}
Qt::ItemFlags DocumentTemplateModel::flags(const QModelIndex &index) const {

	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

}

QModelIndex DocumentTemplateModel::indexFromItem(DocumentItem* item) {

	DocumentItem* parentItem = qobject_cast<DocumentItem*>(item->parent());

	if (parentItem == nullptr) {
		int row = _root->subitems().indexOf(item);

		if (row < 0 or row >= _root->subitems().size()) {
			return QModelIndex();
		} else {
			return createIndex(row,0,item);
		}
	}

	int row = parentItem->subitems().indexOf(item);

	if (row < 0 or row >= _root->subitems().size()) {
		return QModelIndex();
	} else {
		return createIndex(row,0,item);
	}

	return QModelIndex();
}

void DocumentTemplateModel::insertItem(QModelIndex const& parent, int pos, DocumentItem* item) {

	DocumentItem* target = nullptr;

	if (parent != QModelIndex()) {
		target = reinterpret_cast<DocumentItem*>(parent.internalPointer());
	}

	int nItems = _root->subitems().size();

	if (target != nullptr) {
		target->subitems().size();
	}

	int tPos = pos;

	if (pos < 0) {
		tPos = (nItems + 1 + pos)%(nItems + 1);
	}

	if (pos > nItems) {
		tPos = nItems;
	}

	beginInsertRows(parent,tPos,tPos);
	if (target != nullptr) {
		target->insertSubItem(item, tPos);
	} else {
		_root->insertSubItem(item, tPos);
	}
	endInsertRows();

}
void DocumentTemplateModel::removeItem(QModelIndex const& itemIndex) {

	if (itemIndex == QModelIndex()) {
		return;
	}

	DocumentItem* target = reinterpret_cast<DocumentItem*>(itemIndex.internalPointer());

	QModelIndex parent = itemIndex.parent();

	int row = 0;

	if (parent == QModelIndex()) {
		row = _root->subitems().indexOf(target);
	} else {
		DocumentItem* pItem = reinterpret_cast<DocumentItem*>(parent.internalPointer());
		row = pItem->subitems().indexOf(target);
	}

	if (row < 0 or row >= rowCount(parent)) {
		return;
	}

	beginRemoveRows(parent, row, row);


	if (parent == QModelIndex()) {
		_root->removeSubItem(row);
	} else {
		DocumentItem* pItem = reinterpret_cast<DocumentItem*>(parent.internalPointer());
		pItem->removeSubItem(row);
	}

	endRemoveRows();

}

void DocumentTemplateModel::setDocumentTemplate(DocumentTemplate* docTemplate) {

	if (_root == docTemplate) {
		return;
	}

	beginResetModel();
	_root = docTemplate;
	endResetModel();
}
