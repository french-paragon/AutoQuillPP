#ifndef MAINWINDOWS_H
#define MAINWINDOWS_H

#include <QMainWindow>
#include <QModelIndex>

namespace AutoQuill {
	class DocumentTemplate;
	class DocumentTemplateModel;
}

class DocumentPreviewWidget;

class QDockWidget;
class QTreeView;
class QPushButton;

class MainWindows : public QMainWindow
{
    Q_OBJECT
public:
    MainWindows(QWidget* parent = nullptr);

	AutoQuill::DocumentTemplate* currentDocumentTemplate() const;
	void setCurrentDocumentTemplate(AutoQuill::DocumentTemplate* docTemplate);

	void openProjectFromFile(QString const& path);

protected:

	struct InsertPos {
		QModelIndex parent;
		int row;
	};

	InsertPos currentInsertPos();

	void refreshNewItemMenu();
	void addDocumentItem(int type, bool topLevel = false);

	void refreshPropertiesWidget();

	void viewItemPage(QModelIndex const& clickedIdx);

	void saveProject();
	void saveProjectAs();
	void openProject();
    void exportDocument();

	QDockWidget* _projectTreeDockWidget;
	QTreeView* _projectTreeViewWidget;
	QPushButton* _newItemButton;

	QDockWidget* _itemPropertiesDockWidget;

	DocumentPreviewWidget* _docPreviewWidget;

	AutoQuill::DocumentTemplate* _currentDocumentTemplate;
	AutoQuill::DocumentTemplateModel* _documentTemplateModel;
};

#endif // MAINWINDOWS_H
