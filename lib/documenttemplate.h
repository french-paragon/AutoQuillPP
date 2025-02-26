#ifndef DOCUMENTTEMPLATE_H
#define DOCUMENTTEMPLATE_H

#include <QObject>
#include <QAbstractItemModel>

class DocumentItem;

class DocumentTemplate : public QObject
{
    Q_OBJECT
public:
	explicit DocumentTemplate(QObject* parent = nullptr);


	bool removeSubItem(int index);
	bool moveSubItem(int previousIndex, int newIndex);
	void insertSubItem(DocumentItem* item, int position = -1);

	inline QList<DocumentItem*> const& subitems() const {
		return _items;
	}

protected:

	QList<DocumentItem*> _items;
};

class DocumentTemplateModel : public QAbstractItemModel {
	Q_OBJECT
public:

	enum SpecialRoles {
		ItemRole = Qt::UserRole+1,
		ItemPageRole = Qt::UserRole+2
	};

	explicit DocumentTemplateModel(QObject* parent = nullptr);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex &index) const;

	QModelIndex indexFromItem(DocumentItem* item);

	void insertItem(QModelIndex const& parent, int pos, DocumentItem* item);
	void removeItem(QModelIndex const& itemIndex);

	void setDocumentTemplate(DocumentTemplate* docTemplate);

protected:
	DocumentTemplate* _root;
};

#endif // DOCUMENTTEMPLATE_H
