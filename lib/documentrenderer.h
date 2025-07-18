#ifndef DOCUMENTRENDERER_H
#define DOCUMENTRENDERER_H

#include <QString>
#include <QPoint>
#include <QSize>
#include <QVector>

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

	struct ItemRenderInfos;

	struct LayoutResults {
		QVector<ItemRenderInfos*> layout;
		RenderingStatus status;
	};

	DocumentRenderer(DocumentTemplate const& docTemplate);
	~DocumentRenderer();

	/*!
	 * \brief layout layout the items and return a list of rendered pages
	 * \param dataInterface the data interface for the document
	 * \param pluginManager the plugin manager to use for the plugins.
	 * \return the LayoutResults, containing the list of pages, as well as the layout status
	 */
	LayoutResults layout(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager);
	RenderingStatus render(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager, QString const& filename);

	RenderingStatus render(QVector<ItemRenderInfos*> const& layout, RenderPluginManager const& pluginManager, QString const& filename);


	struct ItemRenderInfos {

		ItemRenderInfos() {
			toRender = true;
		}

		~ItemRenderInfos() {
			for (ItemRenderInfos* itemRenderInfos : qAsConst(subitemsRenderInfos)) {
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
		QVector<ItemRenderInfos*> subitemsRenderInfos;

		/*!
		 * \brief translate translate the current item, and all subitems
		 * \param delta the delta to apply to the origin.
		 */
		void translate(QPointF const& delta);
	};

protected :

	struct RenderContext {
		DocumentItem::Direction direction;
		QPointF origin;
		QSizeF region;
	};

	RenderingStatus layoutDocument(QVector<ItemRenderInfos*> & pages, DocumentDataInterface const* dataInterface);
	RenderingStatus layoutItem(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr, QVector<ItemRenderInfos*>* targetItemPool = nullptr);

	RenderingStatus layoutCondition(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutLoop(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutPage(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr, QVector<ItemRenderInfos*>* targetItemPool = nullptr);
	RenderingStatus layoutList(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutFrame(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutText(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutImage(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);
	RenderingStatus layoutPlugin(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender = nullptr);

	RenderingStatus renderItem(ItemRenderInfos& itemInfos);

	RenderingStatus renderCondition(ItemRenderInfos& itemInfos);
	RenderingStatus renderLoop(ItemRenderInfos& itemInfos);
	RenderingStatus renderPage(ItemRenderInfos& itemInfos);
	RenderingStatus renderList(ItemRenderInfos& itemInfos);
	RenderingStatus renderFrame(ItemRenderInfos& itemInfos);
	RenderingStatus renderText(ItemRenderInfos& itemInfos);
	RenderingStatus renderImage(ItemRenderInfos& itemInfos);
	RenderingStatus renderPlugin(ItemRenderInfos& itemInfos);

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
