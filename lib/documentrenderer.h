#ifndef DOCUMENTRENDERER_H
#define DOCUMENTRENDERER_H

#include <QString>
#include <QPoint>
#include <QSize>

class QPainter;
class QPdfWriter;

#include "./documentitem.h"
#include "./documentdatainterface.h"

namespace AutoQuill {

class DocumentTemplate;
class DocumentDataInterface;
class RenderPluginManager;

class DocumentRenderer
{
public :

	enum Status {
		Success, //the item was fully rendered
		NotAllItemsRendered, //the item still has subitem to render, this may trigger a new page if the item and parents OverflowBehavior
		//all errors below will have the layout fail
		MissingModel,
		MissingData,
		MissingSpace,
		OtherError
	};

	struct RenderingStatus {
		Status status;
		QString message;
		QSizeF renderSize;
	};

	DocumentRenderer(DocumentTemplate const& docTemplate);
	~DocumentRenderer();

	RenderingStatus render(DocumentDataInterface* dataInterface, RenderPluginManager const& pluginManager, QString const& filename);

protected :

	struct RenderContext {
		DocumentItem::Direction direction;
		QPointF origin;
		QSizeF region;
	};

	struct itemRenderInfos {

		itemRenderInfos() {
			toRender = true;
		}

		~itemRenderInfos() {
			for (itemRenderInfos* itemRenderInfos : qAsConst(subitemsRenderInfos)) {
				delete itemRenderInfos;
			}
		}

		DocumentValue itemValue;
		DocumentItem* item;
		QPointF currentOrigin;
		QSizeF currentSize;
		QSizeF maxSize;
		Status layoutStatus;
		Status renderStatus;
		bool toRender;
		bool rendered;
		QVariant continuationIndex;
		QVector<itemRenderInfos*> subitemsRenderInfos;
	};

	RenderingStatus layoutDocument(QVector<itemRenderInfos*> & pages, DocumentDataInterface* dataInterface);
	RenderingStatus layoutItem(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr, QVector<itemRenderInfos*>* targetItemPool = nullptr);

	RenderingStatus layoutCondition(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutLoop(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutPage(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr, QVector<itemRenderInfos*>* targetItemPool = nullptr);
	RenderingStatus layoutList(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutFrame(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutText(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutImage(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutPlugin(itemRenderInfos& itemInfos, itemRenderInfos* previousRender = nullptr);

	RenderingStatus renderItem(itemRenderInfos& itemInfos);

	RenderingStatus renderCondition(itemRenderInfos& itemInfos);
	RenderingStatus renderLoop(itemRenderInfos& itemInfos);
	RenderingStatus renderPage(itemRenderInfos& itemInfos);
	RenderingStatus renderList(itemRenderInfos& itemInfos);
	RenderingStatus renderFrame(itemRenderInfos& itemInfos);
	RenderingStatus renderText(itemRenderInfos& itemInfos);
	RenderingStatus renderImage(itemRenderInfos& itemInfos);
	RenderingStatus renderPlugin(itemRenderInfos& itemInfos);

	QPainter* _painter;
	QPdfWriter* _writer;
	int _pagesWritten;
	int _pagesToWrite;

	DocumentTemplate const* _docTemplate;

	RenderPluginManager const* _pluginManager;
	RenderContext _renderContext;
};

} // namespace AutoQuill

#endif // DOCUMENTRENDERER_H
