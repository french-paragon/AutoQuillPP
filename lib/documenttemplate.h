#ifndef DOCUMENTTEMPLATE_H
#define DOCUMENTTEMPLATE_H

#include <QObject>
#include <QAbstractItemModel>

#include <QJsonValue>

namespace AutoQuill {

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

    QJsonValue encapsulateToJson() const;
	bool configureFromJson(QJsonValue const& value);

	bool saveTo(QString const& path);
	bool loadFrom(QString const& path);

    inline QString currentSavePath() const {
        return _currentSavePath;
    }

    inline void setCurrentSavePath(QString const& path) {
        if (path != _currentSavePath) {
            _currentSavePath = path;
        }
    }

Q_SIGNALS:

	void aboutToBeReset();
	void reseted();

protected:

	QList<DocumentItem*> _items;

    QString _currentSavePath;
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

} // namespace AutoQuill

#endif // DOCUMENTTEMPLATE_H
