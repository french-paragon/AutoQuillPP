#ifndef MAINWINDOWS_H
#define MAINWINDOWS_H

#include <QMainWindow>
#include <QModelIndex>

class DocumentTemplate;
class DocumentTemplateModel;

class QDockWidget;
class QTreeView;
class QPushButton;

class MainWindows : public QMainWindow
{
    Q_OBJECT
public:
    MainWindows(QWidget* parent = nullptr);

	DocumentTemplate* currentDocumentTemplate() const;
	void setCurrentDocumentTemplate(DocumentTemplate* docTemplate);

protected:

	struct InsertPos {
		QModelIndex parent;
		int row;
	};

	InsertPos currentInsertPos();

	void refreshNewItemMenu();
	void addDocumentItem(int type, bool topLevel = false);

	void refreshPropertiesWidget();

	QDockWidget* _projectTreeDockWidget;
	QTreeView* _projectTreeViewWidget;
	QPushButton* _newItemButton;

	QDockWidget* _itemPropertiesDockWidget;

	DocumentTemplate* _currentDocumentTemplate;
	DocumentTemplateModel* _documentTemplateModel;
};

#endif // MAINWINDOWS_H
