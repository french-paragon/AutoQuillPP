#ifndef DOCUMENTRENDERER_H
#define DOCUMENTRENDERER_H

#include <QString>
#include <QPoint>
#include <QSize>
#include <QVector>

class QPainter;
class QPdfWriter;
class QIODevice;

#include "./documentitem.h"
#include "./documentdatainterface.h"

namespace AutoQuill {

class DocumentTemplate;
class DocumentDataInterface;
class RenderPluginManager;

struct ItemRenderInfos;

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
     *
     * Pay attention, the ItemRenderInfos in the layout keep a reference to the DocumentTemplate, so you need to ensure the
     * document template is not destroyed before you are done using the layout!
     */
    LayoutResults layout(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager);
    /*!
     * \brief layoutHeadless does the same thing as layout, but using an external QPainter. Use this if you are using the renderer to paint to external QtSurfaces
     * \param dataInterface the data interface to use
     * \param pluginManager the plugin manager to use
     * \param painterOverride the painter to use
     * \return the LayoutResults, containing the list of pages, as well as the layout status
     *
     * Pay attention, the ItemRenderInfos in the layout keep a reference to the DocumentTemplate, so you need to ensure the
     * document template is not destroyed before you are done using the layout!
     */
    LayoutResults layoutHeadless(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager, QPainter* painterOverride);
	RenderingStatus render(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager, QIODevice* device);
	RenderingStatus render(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager, QString const& filename);

    /*!
     * \brief render render the elements in a given layout
     * \param layout the layout to render (the function will not take ownership and will not delete the layout)
     * \param pluginManager the plugin manager to use
     * \param device the device to render to
     * \return a rendering status
     */
    RenderingStatus render(QVector<ItemRenderInfos*> const& layout,
                           RenderPluginManager const& pluginManager,
                           QIODevice* device);
    /*!
     * \brief render render the elements in a given layout
     * \param layout the layout to render (the function will not take ownership and will not delete the layout)
     * \param pluginManager the plugin manager to use
     * \param filename the file to render to
     * \return a rendering status
     */
    RenderingStatus render(QVector<ItemRenderInfos*> const& layout,
                           RenderPluginManager const& pluginManager,
                           QString const& filename);

    /*!
     * \brief renderItem render an item using a specific QPainter
     * \param itemInfos the item to render
     * \param painterOverride the painter to paint to
     * \return a rendering status
     *
     * This function is meant to be used for previewing items, not to render the document.
     * It will not assume there is a paged device underneath, so you need to ensure
     * only one page is rendered.
     */
    RenderingStatus renderItemToExternalPainter(ItemRenderInfos& itemInfos, QPainter* painterOverride);

    static int getLayoutNPages(QVector<ItemRenderInfos*> const& layout);
    static ItemRenderInfos* getLayoutNthPage(QVector<ItemRenderInfos*> const& layout, int n);

protected :

	struct RenderContext {
		DocumentItem::Direction direction;
		QPointF origin;
		QSizeF region;
	};

    RenderingStatus layoutDocument(QVector<ItemRenderInfos*> & layout, DocumentDataInterface const* dataInterface);
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
    DocumentRenderer::Status layoutStatus;
    DocumentRenderer::Status renderStatus;
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

} // namespace AutoQuill

#endif // DOCUMENTRENDERER_H
