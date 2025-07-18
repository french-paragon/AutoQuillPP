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

	/*!
	 * \brief REF_URL_SEP is the separator used to build references url.
	 */
	static constexpr char REF_URL_SEP = '/';

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

	DocumentItem* findByReference(QString const& ref) const;

Q_SIGNALS:

	void aboutToBeReset();
	void reseted();

protected:

	QList<DocumentItem*> _items;

    QString _currentSavePath;

	friend class DocumentTemplateModel;
};

class DocumentTemplateModel : public QAbstractItemModel {
	Q_OBJECT
public:

	enum SpecialRoles {
		ItemRole = Qt::UserRole+1,
		ItemPageRole = Qt::UserRole+2
	};

	explicit DocumentTemplateModel(QObject* parent = nullptr);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;

	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList &indexes) const override;

	QModelIndex indexFromItem(DocumentItem* item);

	void insertItem(QModelIndex const& parent, int pos, DocumentItem* item);
	void removeItem(QModelIndex const& itemIndex);
	void moveItem(QModelIndex const& itemIndex, int pos, QModelIndex const& parent);

	void setDocumentTemplate(DocumentTemplate* docTemplate);

protected:
	DocumentTemplate* _root;
};

} // namespace AutoQuill

#endif // DOCUMENTTEMPLATE_H
