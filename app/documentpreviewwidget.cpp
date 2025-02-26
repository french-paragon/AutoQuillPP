#include "documentpreviewwidget.h"

#include "../lib/documenttemplate.h"
#include "../lib/documentitem.h"

#include <QWheelEvent>
#include <QPainter>

#include <QGuiApplication>
#include <QPixmap>
#include <QIcon>

const qreal DocumentPreviewWidget::scaleBase = 2;
const qreal DocumentPreviewWidget::scaleLevelMax = 4;
const qreal DocumentPreviewWidget::scaleLevelMin = -4;

DocumentPreviewWidget::DocumentPreviewWidget(QWidget *parent)
	: QWidget{parent},
	  _scale_level(0),
	  _docMargin(25),
	  _pageMargin(25),
	  _viewOrigin(0,0),
	  _documentTemplate(nullptr),
	  _previously_pressed(Qt::MouseButton::NoButton)
{

}

void DocumentPreviewWidget::setDocumentTemplate(DocumentTemplate* docTemplate) {

	if (_documentTemplate == docTemplate) {
		return;
	}

	_documentTemplate = docTemplate;
	gotoPage(0);
	repaint();

}

QSizeF DocumentPreviewWidget::scrollableRegion() const {

	qreal doc_width = 0;
	qreal doc_height = 0;

	if (_documentTemplate != nullptr) {
		for (int i = 0; i < _documentTemplate->subitems().size(); i++) {
			DocumentItem* item = _documentTemplate->subitems()[i];

			while (item->getType() != DocumentItem::Page) {
				if (item->subitems().isEmpty()) {
					item = nullptr;
					break;
				}
				item = item->subitems()[0];
			}

			if (item == nullptr) {
				doc_height += 2*_pageMargin;
			} else {
				doc_height += 2*_pageMargin + item->initialHeight();
				doc_width = std::max(doc_width, item->initialWidth());
			}
		}
	}

	doc_width += 2*_pageMargin;

	qreal scaleFactor = scale();

	doc_width *= scaleFactor;
	doc_height *= scaleFactor;

	return QSizeF(2*_docMargin+doc_width, 2*_docMargin+doc_height);

}

int DocumentPreviewWidget::nPages() const {

	if (_documentTemplate == nullptr) {
		return 0;
	}

	return _documentTemplate->subitems().size();
}
void DocumentPreviewWidget::gotoPage(int pageId) {

	qreal pos_horz = 0;
	qreal pos_vert = 0;

	if (_documentTemplate == nullptr) {
		return;
	}

	if (pageId >= _documentTemplate->subitems().size() or pageId < 0) {
		return;
	}

	int target = std::min(_documentTemplate->subitems().size()-1, pageId);

	for (int i = 0; i < target; i++) {

		DocumentItem* item = _documentTemplate->subitems()[i];

		while (item->getType() != DocumentItem::Page) {

			if (item->subitems().isEmpty()) {
				item = nullptr;
				break;
			}

			item = item->subitems()[0];
		}

		if (item == nullptr) {
			pos_vert += 2*_pageMargin;
		} else {
			pos_vert += 2*_pageMargin + item->initialHeight();
		}
	}

	DocumentItem* item = _documentTemplate->subitems()[pageId];

	while (item->getType() != DocumentItem::Page) {
		if (item->subitems().isEmpty()) {
			item = nullptr;
			break;
		}
		item = item->subitems()[0];
	}

	qreal scale = 1;

	if (height() > 2*_docMargin) {
		scale = qreal(item->initialHeight() + 2*_pageMargin)/
				qreal(height() - 2*_docMargin);
	} else {
		scale = std::pow(scaleBase,scaleLevelMin);
	}

	qreal scaleLevel = std::log(scale)/std::log(scaleBase);

	setViewOrigin(QPointF(pos_horz, pos_vert));
	setZoomLevel(scaleLevel);
	repaint();

}

void DocumentPreviewWidget::paintEvent(QPaintEvent *event) {

	QPainter painter(this);

	QSize s = rect().size();
	painter.fillRect(0, 0, s.width(), s.height(), QColor(120, 120, 120));

	if (_documentTemplate == nullptr) {
		return;
	}

	qreal pos_horz = 0;
	qreal pos_vert = 0;

	for (int i = 0; i < _documentTemplate->subitems().size(); i++) {

		QPointF offset(pos_horz, pos_vert);

		paintPage(offset, i, painter);

		DocumentItem* item = _documentTemplate->subitems()[i];

		while (item->getType() != DocumentItem::Page) {

			if (item->subitems().isEmpty()) {
				item = nullptr;
				break;
			}

			item = item->subitems()[0];
		}

		if (item == nullptr) {
			pos_vert += 2*_pageMargin;
		} else {
			pos_vert += 2*_pageMargin + item->initialHeight();
		}

	}

}
void DocumentPreviewWidget::keyPressEvent(QKeyEvent *event) {
	QWidget::keyPressEvent(event);
}
void DocumentPreviewWidget::inputMethodEvent(QInputMethodEvent *event) {
	QWidget::inputMethodEvent(event);
}
void DocumentPreviewWidget::wheelEvent(QWheelEvent *event) {

	QPoint d = event->angleDelta();
	if (d.y() == 0){
		event->ignore();
		return;
	}

	if (event->modifiers() == Qt::KeyboardModifier::NoModifier) {

		qreal delta = d.y()/10;
		QPointF posDelta(0, delta);

		setViewOrigin(viewOrigin() + posDelta);

		event->accept();

	} else if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier) {

		qreal delta = d.y()/10;
		QPointF posDelta(delta, 0);

		setViewOrigin(viewOrigin() + posDelta);

		event->accept();

	} else if (event->modifiers() & Qt::KeyboardModifier::ControlModifier) {

		qreal delta = d.y()/500.;
		if (delta == 0) {
			delta = (d.y() > 0) - (d.y() < 0);
		}

		#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
		QPointF mousePos = event->position();
		#else
		QPointF mousePos = event->pos();
		#endif

		QPointF initialDocPos = widget2docPoint(mousePos);

		zoomOut(delta);

		QPointF currentDocPos = widget2docPoint(mousePos);
		QPointF posDelta = currentDocPos - initialDocPos;

		setViewOrigin(viewOrigin() - posDelta);

		event->accept();
	} else {
		event->ignore();
	}
}
void DocumentPreviewWidget::mousePressEvent(QMouseEvent *event) {

	Qt::KeyboardModifiers kmods = event->modifiers();

	if (kmods & Qt::ControlModifier) {
		_control_pressed_when_clicked = true;
	} else {
		_control_pressed_when_clicked = false;
	}

	_previously_pressed = event->buttons();

	if (_previously_pressed == Qt::LeftButton or _previously_pressed == Qt::MiddleButton) {
		_motion_origin_pos = event->pos();
		_motion_origin_t = viewOrigin();

		event->accept();
	} else {
		event->ignore();
	}

}
void DocumentPreviewWidget::mouseReleaseEvent(QMouseEvent *event) {

	_previously_pressed = event->buttons();
	event->ignore();

}
void DocumentPreviewWidget::mouseMoveEvent(QMouseEvent *event) {

	if (_previously_pressed == Qt::MiddleButton) {
		QPoint posDelta = event->pos() - _motion_origin_pos;
		posDelta *= scale();
		setViewOrigin(_motion_origin_t - posDelta);
		event->accept();
	} else {
		event->ignore();
	}
}

void DocumentPreviewWidget::paintPage(QPointF offset, int pageId, QPainter& painter) {

	if (_documentTemplate == nullptr) {
		return;
	}

	DocumentItem* item = _documentTemplate->subitems()[pageId];

	while (item->getType() != DocumentItem::Page) {

		if (item->subitems().isEmpty()) {
			item = nullptr;
			break;
		}

		item = item->subitems()[0];
	}

	if (item == nullptr) {
		return;
	}

	QPointF origin = offset + QPointF(_pageMargin, _pageMargin);

	QRectF pageRect(origin,
					QSizeF(item->initialWidth(), item->initialHeight()));

	QRectF widgetRect = doc2widgetRect(pageRect);

	if (!widgetRect.intersects(rect())) {
		return; //page not visible
	}

	painter.fillRect(widgetRect, QColor(255,255,255));

	QTransform initial = painter.worldTransform();
	painter.translate(widgetRect.topLeft());

	for (int i = 0; i < item->subitems().size(); i++) {
		paintItem(item->subitems()[i], painter);
	}

	painter.setWorldTransform(initial); //reset the transform;
}
void DocumentPreviewWidget::paintItem(DocumentItem* item, QPainter& painter) {

	constexpr int iconSize = 30;

	static QPixmap conditionIcon = QIcon(":/icons/condition.svg").pixmap(iconSize, iconSize);
	static QPixmap frameIcon = QIcon(":/icons/frame.svg").pixmap(iconSize, iconSize);
	static QPixmap imageIcon = QIcon(":/icons/image.svg").pixmap(iconSize, iconSize);
	static QPixmap listIcon = QIcon(":/icons/list.svg").pixmap(iconSize, iconSize);
	static QPixmap loopIcon = QIcon(":/icons/loop.svg").pixmap(iconSize, iconSize);
	static QPixmap pluginIcon = QIcon(":/icons/plugin.svg").pixmap(iconSize, iconSize);
	static QPixmap textIcon = QIcon(":/icons/text.svg").pixmap(iconSize, iconSize);

	DocumentItem::Type type = item->getType();

	auto iconSelect = [&] (DocumentItem::Type type) -> QPixmap& {

		switch(type) {
		case DocumentItem::Condition:
			return conditionIcon;
		case DocumentItem::Image:
			return imageIcon;
		case DocumentItem::List:
			return listIcon;
		case DocumentItem::Loop:
			return loopIcon;
		case DocumentItem::Plugin:
			return pluginIcon;
		case DocumentItem::Text:
			return textIcon;
		case DocumentItem::Frame:
		default:
			return frameIcon;
		}
	};

	QPixmap& icon = iconSelect(type);

	QPen borderPen;
	borderPen.setColor(QColor(44,87,164));
	borderPen.setWidth(3);
	borderPen.setStyle(Qt::DashLine);

	QPointF pos(item->posX(), item->posY());
	qreal s = 1/scale();
	pos *= s;

	switch(type) {
	case DocumentItem::Frame:
	case DocumentItem::Image:
	case DocumentItem::List:
	case DocumentItem::Loop:
	case DocumentItem::Plugin:
	case DocumentItem::Text: {
		QRectF rect(pos, s*QSizeF(item->initialWidth(), item->initialHeight()));

		painter.setPen(borderPen);
		painter.drawRect(rect);
		painter.drawPixmap(pos,icon);
		painter.drawText(pos + QPointF(iconSize,iconSize), item->objectName());
	}
		break;
	default:
		painter.setPen(borderPen);
		painter.drawPixmap(pos,icon);
		painter.drawText(pos + QPointF(iconSize,iconSize), item->objectName());
	}


	QTransform initial = painter.worldTransform();
	painter.translate(pos);

	for (int i = 0; i < item->subitems().size(); i++) {
		paintItem(item->subitems()[i], painter);
	}

	painter.setWorldTransform(initial); //reset the transform;

}
