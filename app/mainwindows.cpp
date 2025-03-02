#include "mainwindows.h"

#include <QDockWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenuBar>
#include <QMenu>

#include "../lib/documentitem.h"
#include "../lib/documenttemplate.h"

#include "documentpreviewwidget.h"

MainWindows::MainWindows(QWidget *parent) :
	QMainWindow(parent),
	_currentDocumentTemplate(nullptr)
{
	DocumentTemplate* documentTemplate = new DocumentTemplate(this);

    //setup menu

    QMenu* fileMenu = menuBar()->addMenu(tr("file"));
    QAction* saveAction = fileMenu->addAction(tr("save"));
    QAction* saveAsAction = fileMenu->addAction(tr("save as"));

    saveAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));

    connect(saveAction, &QAction::triggered, this, &MainWindows::saveProject);
    connect(saveAsAction, &QAction::triggered, this, &MainWindows::saveProjectAs);

	//setup dockers

	_projectTreeDockWidget = new QDockWidget(tr("Project tree"), this);
	_projectTreeDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	QWidget* projectTreeWidget = new QWidget(_projectTreeDockWidget);

	QVBoxLayout* projectTreeDockWidgetLayout = new QVBoxLayout(projectTreeWidget);

	_projectTreeDockWidget->setWidget(projectTreeWidget);

	_projectTreeViewWidget = new QTreeView(_projectTreeDockWidget);
	_projectTreeViewWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	_projectTreeViewWidget->header()->setVisible(false);

	connect(_projectTreeViewWidget, &QTreeView::clicked, this, &MainWindows::refreshNewItemMenu);
	connect(_projectTreeViewWidget, &QTreeView::clicked, this, &MainWindows::refreshPropertiesWidget);
	connect(_projectTreeViewWidget, &QTreeView::clicked, this, &MainWindows::viewItemPage);

	projectTreeDockWidgetLayout->addWidget(_projectTreeViewWidget);

	QHBoxLayout* projectTreeDockWidgetToolsLayout = new QHBoxLayout();
	projectTreeDockWidgetToolsLayout->setMargin(5);
	projectTreeDockWidgetToolsLayout->setSpacing(5);

	_newItemButton = new QPushButton(tr("new..."), _projectTreeDockWidget);
	projectTreeDockWidgetToolsLayout->addWidget(_newItemButton);
	projectTreeDockWidgetToolsLayout->addStretch();

	refreshNewItemMenu();

	projectTreeDockWidgetLayout->addLayout(projectTreeDockWidgetToolsLayout);

	addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, _projectTreeDockWidget);

	_itemPropertiesDockWidget = new QDockWidget(tr("Item properties"), this);
	_itemPropertiesDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, _itemPropertiesDockWidget);

	_documentTemplateModel = new DocumentTemplateModel(this);
	_projectTreeViewWidget->setModel(_documentTemplateModel);

	//setup main widgets

	_docPreviewWidget = new DocumentPreviewWidget(this);

	setCentralWidget(_docPreviewWidget);

	//register the documentTemplate
	setCurrentDocumentTemplate(documentTemplate);

}


DocumentTemplate* MainWindows::currentDocumentTemplate() const {
	return _currentDocumentTemplate;
}

void MainWindows::setCurrentDocumentTemplate(DocumentTemplate* docTemplate) {

	if (_currentDocumentTemplate != docTemplate) {

		_currentDocumentTemplate = docTemplate;

		_documentTemplateModel->setDocumentTemplate(docTemplate);
		_docPreviewWidget->setDocumentTemplate(docTemplate);
	}
}

MainWindows::InsertPos MainWindows::currentInsertPos() {

	QModelIndex idx = _projectTreeViewWidget->currentIndex();

	if (idx == QModelIndex()) {
		return {QModelIndex(), -1};
	}

	DocumentItem* item = qvariant_cast<DocumentItem*>(idx.data(DocumentTemplateModel::ItemRole));

	if (item == nullptr) {
		return {QModelIndex(), -1};
	}

	DocumentItem::Type type = item->getType();

	if (DocumentItem::typeAcceptChildrens(type)) {
		return {idx, -1};
	}

	QModelIndex idxp = idx.parent();

	return {idxp, idx.row() +1};

}

void MainWindows::refreshNewItemMenu() {

	InsertPos insertPos = currentInsertPos();

	QList<DocumentItem::Type> creableTypes;

	if (insertPos.parent == QModelIndex()) {
		creableTypes = DocumentItem::supportedRootTypes();
	} else {
		DocumentItem* item = qvariant_cast<DocumentItem*>(insertPos.parent.data(DocumentTemplateModel::ItemRole));

		if (item != nullptr) {
			creableTypes = item->supportedSubTypes();
		}
	}

	if (_newItemButton->menu() == nullptr) {
		QMenu* menu = new QMenu(_newItemButton);
		_newItemButton->setMenu(menu);
	}

	_newItemButton->menu()->clear();

	for (DocumentItem::Type & t : creableTypes) {

		QIcon icon = DocumentItem::iconForType(t);
		QString text = tr("%1").arg(DocumentItem::typeToString(t));

		QAction* createItemAction = _newItemButton->menu()->addAction(icon, text);

		connect(createItemAction, &QAction::triggered, this, [this, t] () {
			addDocumentItem(t);
		});
	}

	if (insertPos.parent != QModelIndex()) {
		_newItemButton->menu()->addSeparator();

		creableTypes = DocumentItem::supportedRootTypes();

		for (DocumentItem::Type & t : creableTypes) {

			QIcon icon = DocumentItem::iconForType(t);
			QString text = tr("Top Level %1").arg(DocumentItem::typeToString(t));

			QAction* createItemAction = _newItemButton->menu()->addAction(icon, text);

			connect(createItemAction, &QAction::triggered, this, [this, t] () {
				constexpr bool topLevel = true;
				addDocumentItem(t, topLevel);
			});
		}

	}

}
void MainWindows::addDocumentItem(int t, bool topLevel) {

	DocumentItem::Type type = static_cast<DocumentItem::Type>(t);

	InsertPos insertPos = (topLevel) ? InsertPos{QModelIndex(), -1} : currentInsertPos();

	DocumentItem* parentItem = nullptr;
	QList<DocumentItem::Type> creableTypes;

	if (insertPos.parent == QModelIndex()) {
		creableTypes = DocumentItem::supportedRootTypes();
	} else {
		 parentItem = qvariant_cast<DocumentItem*>(insertPos.parent.data(DocumentTemplateModel::ItemRole));

		if (parentItem != nullptr) {
			creableTypes = parentItem->supportedSubTypes();
		}
	}

	if (!creableTypes.contains(type)) {
		return;
	}

	DocumentItem* docItem = new DocumentItem(type, _currentDocumentTemplate);

	_documentTemplateModel->insertItem(insertPos.parent, insertPos.row, docItem);
}

void MainWindows::refreshPropertiesWidget() {

	QWidget* widget = _itemPropertiesDockWidget->widget();

	if (widget != nullptr) {
		widget->deleteLater();
	}

	widget = new QWidget(_itemPropertiesDockWidget);
	_itemPropertiesDockWidget->setWidget(widget);

	QModelIndex currentIdx = _projectTreeViewWidget->currentIndex();

	if (currentIdx == QModelIndex()) {
		return;
	}

	DocumentItem* item = qvariant_cast<DocumentItem*>(currentIdx.data(DocumentTemplateModel::ItemRole));

	if (item == nullptr) {
		return;
	}

	QFormLayout* layout = new QFormLayout(widget);

	QLineEdit* nameEditLine = new QLineEdit(widget);

	nameEditLine->setText(item->objectName());
	connect(nameEditLine, &QLineEdit::textChanged, item, &QObject::setObjectName);

	layout->addRow(tr("Name"), nameEditLine);

	DocumentItem::Type type = item->getType();

	if (type != DocumentItem::Page and item->parentDocumentItem() != nullptr) {

		QDoubleSpinBox* posXEdit = new QDoubleSpinBox(widget);
		QDoubleSpinBox* posYEdit = new QDoubleSpinBox(widget);

		posXEdit->setMinimum(-99999);
		posXEdit->setMaximum(99999);

		posYEdit->setMinimum(-99999);
		posYEdit->setMaximum(99999);

		posXEdit->setValue(item->posX());
		posYEdit->setValue(item->posY());

		connect(posXEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
				item, &DocumentItem::setPosX);

		connect(posYEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
				item, &DocumentItem::setPosY);

		layout->addRow(tr("Position X"), posXEdit);
		layout->addRow(tr("Position Y"), posYEdit);

	}

	if ((type != DocumentItem::Loop and type != DocumentItem::Condition) or
			item->parentDocumentItem() != nullptr) {

		QDoubleSpinBox* widthEdit = new QDoubleSpinBox(widget);
		QDoubleSpinBox* heightEdit = new QDoubleSpinBox(widget);

		widthEdit->setMinimum(0);
		widthEdit->setMaximum(99999);

		heightEdit->setMinimum(0);
		heightEdit->setMaximum(99999);

		widthEdit->setValue(item->initialWidth());
		heightEdit->setValue(item->initialHeight());

		connect(widthEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
				item, &DocumentItem::setInitialWidth);

		connect(heightEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
				item, &DocumentItem::setInitialHeight);

		layout->addRow(tr("Initial Width"), widthEdit);
		layout->addRow(tr("Initial Height"), heightEdit);

		if (type != DocumentItem::Page) {
			QDoubleSpinBox* maxWidthEdit = new QDoubleSpinBox(widget);
			QDoubleSpinBox* maxHeightEdit = new QDoubleSpinBox(widget);

			maxWidthEdit->setMinimum(0);
			maxWidthEdit->setMaximum(99999);

			maxHeightEdit->setMinimum(0);
			maxHeightEdit->setMaximum(99999);

			maxWidthEdit->setValue(item->initialWidth());
			maxHeightEdit->setValue(item->initialHeight());

			connect(maxWidthEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
					item, &DocumentItem::setMaxWidth);

			connect(maxHeightEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
					item, &DocumentItem::setMaxHeight);

			layout->addRow(tr("Max Width"), maxWidthEdit);
			layout->addRow(tr("Max Height"), maxHeightEdit);
		}

	}


}

void MainWindows::viewItemPage(QModelIndex const& clickedIdx) {

	if (!clickedIdx.isValid()) {
		return;
	}

	bool ok;
	int pageId = clickedIdx.data(DocumentTemplateModel::ItemPageRole).toInt(&ok);

	if (!ok) {
		return;
	}

	if (pageId < 0) {
		return;
	}

	_docPreviewWidget->gotoPage(pageId);

}

void MainWindows::saveProject() {

    if (_currentDocumentTemplate == nullptr) {
        return;
    }

    if (!_currentDocumentTemplate->currentSavePath().isEmpty()) {
        bool ok = _currentDocumentTemplate->saveTo(_currentDocumentTemplate->currentSavePath());

        if (ok) {
            return;
        }
    }

    saveProjectAs();
}
void MainWindows::saveProjectAs() {

    if (_currentDocumentTemplate == nullptr) {
        return;
    }

    QString filePath =
        QFileDialog::getSaveFileName(this,
                                    tr("Save template as"),
                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    if (filePath.isEmpty()) {
        return;
    }

    _currentDocumentTemplate->setCurrentSavePath(filePath);

    _currentDocumentTemplate->saveTo(_currentDocumentTemplate->currentSavePath());
}
