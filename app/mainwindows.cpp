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
#include <QButtonGroup>
#include <QPushButton>
#include <QFontComboBox>

#include "../lib/documentitem.h"
#include "../lib/documenttemplate.h"

#include "documentpreviewwidget.h"
#include "exportactions.h"

MainWindows::MainWindows(QWidget *parent) :
	QMainWindow(parent),
	_currentDocumentTemplate(nullptr)
{
	DocumentTemplate* documentTemplate = new DocumentTemplate(this);

    //setup menu

    QMenu* fileMenu = menuBar()->addMenu(tr("file"));
    QAction* saveAction = fileMenu->addAction(tr("save"));
    QAction* saveAsAction = fileMenu->addAction(tr("save as"));
    QAction* openAction = fileMenu->addAction(tr("open"));
    QAction* exportAction = fileMenu->addAction(tr("export"));

    saveAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
	openAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    exportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));

    connect(saveAction, &QAction::triggered, this, &MainWindows::saveProject);
    connect(saveAsAction, &QAction::triggered, this, &MainWindows::saveProjectAs);
	connect(openAction, &QAction::triggered, this, &MainWindows::openProject);
    connect(exportAction, &QAction::triggered, this, &MainWindows::exportDocument);

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

            maxWidthEdit->setValue(item->maxWidth());
            maxHeightEdit->setValue(item->maxHeight());

			connect(maxWidthEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
					item, &DocumentItem::setMaxWidth);

			connect(maxHeightEdit, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged),
					item, &DocumentItem::setMaxHeight);

			layout->addRow(tr("Max Width"), maxWidthEdit);
			layout->addRow(tr("Max Height"), maxHeightEdit);
		}

	}

	if (type == DocumentItem::Text) {

		QFontComboBox* fontSelect = new QFontComboBox(widget);

		if (!item->fontName().isEmpty()) {
			QFont font(item->fontName());
			fontSelect->setCurrentFont(font);
		} else {
			QFont font("serif");
			fontSelect->setCurrentFont(font);
		}

		connect(fontSelect, &QFontComboBox::currentFontChanged, item, [item] (QFont const& font) {
			item->setFontName(font.family());
		});

		layout->addRow(tr("Font"), fontSelect);

		QComboBox* weightSelect = new QComboBox(widget);

		weightSelect->addItem(tr("Thin"), DocumentItem::TextWeight::Thin);
		weightSelect->addItem(tr("ExtraLight"), DocumentItem::TextWeight::ExtraLight);
		weightSelect->addItem(tr("Light"), DocumentItem::TextWeight::Light);
		weightSelect->addItem(tr("Normal"), DocumentItem::TextWeight::Normal);
		weightSelect->addItem(tr("Medium"), DocumentItem::TextWeight::Medium);
		weightSelect->addItem(tr("DemiBold"), DocumentItem::TextWeight::DemiBold);
		weightSelect->addItem(tr("Bold"), DocumentItem::TextWeight::Bold);
		weightSelect->addItem(tr("ExtraBold"), DocumentItem::TextWeight::ExtraBold);
		weightSelect->addItem(tr("Black"), DocumentItem::TextWeight::Black);

		int currentIndex = weightSelect->findData(item->fontWeight());
		weightSelect->setCurrentIndex(currentIndex);

		connect(weightSelect, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), item, [item, weightSelect] () {
			item->setFontWeight(weightSelect->currentData().toInt());
		});

		layout->addRow(tr("Font weight"), weightSelect);

		QDoubleSpinBox* sizeSelect = new QDoubleSpinBox(widget);

		sizeSelect->setMinimum(0);
		sizeSelect->setMaximum(99999);
		sizeSelect->setDecimals(1);
		sizeSelect->setSingleStep(1);

		sizeSelect->setValue(item->fontSize());

		connect(sizeSelect, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), item, [item, sizeSelect] () {
			item->setFontSize(sizeSelect->value());
		});

		layout->addRow(tr("Font size"), sizeSelect);


		QButtonGroup* textAlignmentGroup = new QButtonGroup(widget);

		QPushButton* alignLeftButton = new QPushButton(QIcon(":/icons/align_left.svg"), "", widget);
		QPushButton* alignCenterButton = new QPushButton(QIcon(":/icons/align_center.svg"), "", widget);
		QPushButton* alignRightButton = new QPushButton(QIcon(":/icons/align_right.svg"), "", widget);
		QPushButton* alignJustifyButton = new QPushButton(QIcon(":/icons/align_justify.svg"), "", widget);

		alignLeftButton->setCheckable(true);
		alignCenterButton->setCheckable(true);
		alignRightButton->setCheckable(true);
		alignJustifyButton->setCheckable(true);

		switch(item->textAlign()) {
		case DocumentItem::TextAlign::AlignLeft:
			alignLeftButton->setChecked(true);
			break;
		case DocumentItem::TextAlign::AlignCenter:
			alignCenterButton->setChecked(true);
			break;
		case DocumentItem::TextAlign::AlignRight:
			alignRightButton->setChecked(true);
			break;
		case DocumentItem::TextAlign::AlignJustify:
			alignJustifyButton->setChecked(true);
			break;
		default:
			alignLeftButton->setChecked(true);
		}

		textAlignmentGroup->addButton(alignLeftButton, DocumentItem::TextAlign::AlignLeft);
		textAlignmentGroup->addButton(alignCenterButton, DocumentItem::TextAlign::AlignCenter);
		textAlignmentGroup->addButton(alignRightButton, DocumentItem::TextAlign::AlignRight);
		textAlignmentGroup->addButton(alignJustifyButton, DocumentItem::TextAlign::AlignJustify);

		connect(textAlignmentGroup, &QButtonGroup::idToggled, item, [item] (int id, bool checked) {
			if (checked) {
				item->setTextAlign(static_cast<DocumentItem::TextAlign>(id));
			}
		});

		QHBoxLayout* buttonLayout = new QHBoxLayout();

		buttonLayout->addWidget(alignLeftButton);
		buttonLayout->addWidget(alignCenterButton);
		buttonLayout->addWidget(alignRightButton);
		buttonLayout->addWidget(alignJustifyButton);

		layout->addRow(tr("Text align"), buttonLayout);
	}

    QLineEdit* dataKeyEditLine = new QLineEdit(widget);
    QLineEdit* dataEditLine = new QLineEdit(widget);

    dataKeyEditLine->setText(item->dataKey());
    dataEditLine->setText(item->data());

    connect(dataKeyEditLine, &QLineEdit::textChanged,
            item, &DocumentItem::setDataKey);

    connect(dataEditLine, &QLineEdit::textChanged,
            item, &DocumentItem::setData);

    layout->addRow(tr("Data key"), dataKeyEditLine);
    layout->addRow(tr("Data"), dataEditLine);


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
void MainWindows::openProject() {

	if (_currentDocumentTemplate == nullptr) {
		DocumentTemplate* documentTemplate = new DocumentTemplate(this);
		setCurrentDocumentTemplate(documentTemplate);
	}

	QString filePath =
		QFileDialog::getOpenFileName(this,
									tr("Open template"),
									QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

	if (filePath.isEmpty()) {
		return;
	}

	_currentDocumentTemplate->loadFrom(filePath);
	_currentDocumentTemplate->setCurrentSavePath(filePath);
}

void MainWindows::exportDocument() {

    exportTemplateUsingJson(_currentDocumentTemplate, this);

}
