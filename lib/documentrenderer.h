#ifndef DOCUMENTRENDERER_H
#define DOCUMENTRENDERER_H

#include <QString>
#include <QPoint>
#include <QSize>

class QPainter;
class QPdfWriter;

#include "./documentitem.h"

class DocumentTemplate;
class DocumentDataInterface;
class DocumentValue;

class DocumentRenderer
{
public :

	enum Status {
		Success,
		MissingModel,
		MissingData,
		MissingSpace,
		NotAllItemsRendered,
		OtherError
	};

	struct RenderingStatus {
		Status status;
		QString message;
		QSizeF renderSize;
	};

	DocumentRenderer(DocumentTemplate* docTemplate);
	~DocumentRenderer();

	RenderingStatus render(DocumentDataInterface* dataInterface, QString const& filename);

protected :

	struct RenderContext {
		DocumentItem::Direction direction;
		QPointF origin;
		QSizeF region;
	};

	RenderingStatus renderItem(DocumentItem* item, DocumentValue const& val);

	RenderingStatus renderCondition(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderLoop(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderPage(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderList(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderFrame(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderText(DocumentItem* item, DocumentValue const& val);
	RenderingStatus renderImage(DocumentItem* item, DocumentValue const& val);

	QPainter* _painter;
	QPdfWriter* _writer;

	DocumentTemplate* _docTemplate;

	RenderContext _renderContext;
};

#endif // DOCUMENTRENDERER_H
