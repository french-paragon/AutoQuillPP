#ifndef DOCUMENTPREVIEWWIDGET_H
#define DOCUMENTPREVIEWWIDGET_H

#include <QWidget>

#include <QPoint>
#include <QSize>

#include <cmath>

namespace AutoQuill {
	class DocumentTemplate;
	class DocumentItem;
}

class DocumentPreviewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit DocumentPreviewWidget(QWidget *parent = nullptr);

	void setDocumentTemplate(AutoQuill::DocumentTemplate* docTemplate);

	inline qreal scale() const {
		return std::pow(scaleBase,_scale_level);
	}
	inline void zoomIn(qreal steps) {
		setZoomLevel(_scale_level + steps);
	}
	inline void zoomOut(qreal steps) {
		setZoomLevel(_scale_level - steps);
	}
	inline void setZoomLevel(qreal zoomLevel) {

		qreal level = std::min(scaleLevelMax, std::max(scaleLevelMin, zoomLevel));

		if (level != _scale_level) {
			_scale_level = level;
			Q_EMIT scaleChanged();
			repaint();
		}
	}

	QSizeF scrollableRegion() const;

	/*!
	 * \brief viewOrigin accessor for the origin point for painting (in document coordinates)
	 * \return a point
	 */
	inline QPointF viewOrigin() const {
		return _viewOrigin;
	}
	inline void setViewOrigin(QPointF const& pos) {
		if (_viewOrigin != pos) {
			_viewOrigin = pos;
			Q_EMIT viewOriginChanged();
			repaint();
		}
	}

	int nPages() const;
	void gotoPage(int pageId);

	inline QPointF doc2widgetPoint(QPointF const& docPoint) const {
		QPointF widgetPoint = docPoint;
		widgetPoint -= _viewOrigin;
		widgetPoint /= scale();
		widgetPoint.rx() += _docMargin;
		widgetPoint.ry() += _docMargin;
		return widgetPoint;

	}

	inline QPointF widget2docPoint(QPointF const& widgetPoint) const {
		QPointF docPoint = widgetPoint;
		docPoint.rx() -= _docMargin;
		docPoint.ry() -= _docMargin;
		docPoint *= scale();
		docPoint += _viewOrigin;
		return docPoint;
	}

	inline QRectF doc2widgetRect(QRectF const& docRect) const {
		QPointF topLeft = doc2widgetPoint(docRect.topLeft());
		QPointF bottomRight = doc2widgetPoint(docRect.bottomRight());
		return QRectF(topLeft, bottomRight);

	}

	inline QRectF widget2docRect(QRectF const& widgetRect) const {
		QPointF topLeft = widget2docPoint(widgetRect.topLeft());
		QPointF bottomRight = widget2docPoint(widgetRect.bottomRight());
		return QRectF(topLeft, bottomRight);
	}

	inline QTransform doc2widget() const {
		QTransform doc2widget;
		doc2widget.translate(-_viewOrigin.x(), -_viewOrigin.y());
		qreal s = 1/scale();
		doc2widget.scale(s,s);
		doc2widget.translate(_docMargin, _docMargin);
		return doc2widget;
	}

	inline QTransform widget2doc() const {
		QTransform widget2doc;
		widget2doc.translate(-_docMargin,-_docMargin);
		qreal s = scale();
		widget2doc.scale(s,s);
		widget2doc.translate(_viewOrigin.x(), _viewOrigin.y());
		return widget2doc;
	}

Q_SIGNALS:

	void scaleChanged();
	void scrollableRegionChanged();
	void viewOriginChanged();

protected:

	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void inputMethodEvent(QInputMethodEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

	void paintPage(QPointF offset, int pageId, QPainter& painter);
	void paintItem(AutoQuill::DocumentItem* item, QPainter& painter);

	static const qreal scaleBase;
	static const qreal scaleLevelMax;
	static const qreal scaleLevelMin;
	qreal _scale_level;

	qreal _docMargin; //margins around the whole document, in widget units
	qreal _pageMargin; //margins around a page, in document unit

	QPointF _viewOrigin;

	AutoQuill::DocumentTemplate* _documentTemplate;

private:

	QPoint _motion_origin_pos;
	QPointF _motion_origin_t;
	bool _control_pressed_when_clicked;
	int _previously_pressed;

};

#endif // DOCUMENTPREVIEWWIDGET_H
