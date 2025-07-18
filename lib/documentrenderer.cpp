#include "documentrenderer.h"

#include <QObject>

#include <QPdfWriter>
#include <QPainter>

#include "documenttemplate.h"
#include "documentitem.h"
#include "documentdatainterface.h"
#include "renderplugin.h"

namespace AutoQuill {

DocumentRenderer::DocumentRenderer(const DocumentTemplate &docTemplate) :
	_docTemplate(&docTemplate),
	_writer(nullptr),
	_painter(nullptr)
{

}

DocumentRenderer::~DocumentRenderer() {

	if (_painter != nullptr) {
		delete _painter;
		_painter = nullptr;
	}
	if (_writer != nullptr) {
		delete _writer;
		_writer = nullptr;
	}
}

DocumentRenderer::LayoutResults DocumentRenderer::layout(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager) {

	_pluginManager = &pluginManager;

	QVector<ItemRenderInfos*> layout;

	RenderingStatus layoutStatus = layoutDocument(layout, dataInterface);

	if (layoutStatus.status != Success) {
		for (ItemRenderInfos* item : qAsConst(layout)) {
			if (item != nullptr) {
				delete item;
			}
		}
		return {layout, layoutStatus};
	}

	return {QVector<ItemRenderInfos*>(), layoutStatus};
}
DocumentRenderer::RenderingStatus DocumentRenderer::render(DocumentDataInterface const* dataInterface, RenderPluginManager const& pluginManager, QString const& filename) {

	_pluginManager = &pluginManager;

	if (dataInterface == nullptr) {
		return RenderingStatus{MissingData, QObject::tr("Missing data interface")};
	}

	if (_docTemplate == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid template")};
	}

	if (_writer != nullptr) {
		delete _writer;
	}

	_writer = new QPdfWriter(filename);
	_writer->setResolution(72);
	_writer->setTitle(_docTemplate->objectName());
	_writer->setPageMargins(QMarginsF(0,0,0,0));

	if (_painter != nullptr) {
		delete _painter;
	}

	_painter = new QPainter(_writer);
	_pagesToWrite = 0;
	_pagesWritten = 0;

	RenderingStatus status{Success, ""};

	QVector<ItemRenderInfos*> layout;

	RenderingStatus layoutStatus = layoutDocument(layout, dataInterface);

	if (layoutStatus.status != Success) {
		for (ItemRenderInfos* item : qAsConst(layout)) {
			if (item != nullptr) {
				delete item;
			}
		}
		delete _painter;
		delete _writer;
		_painter = nullptr;
		_writer = nullptr;
		return layoutStatus;
	}

	if (layout.isEmpty()) {
		delete _painter;
		delete _writer;
		_painter = nullptr;
		_writer = nullptr;
		return RenderingStatus{MissingModel, QObject::tr("Final layout is empty")};
	}

	for (ItemRenderInfos* item : layout) {

		RenderingStatus itemStatus = renderItem(*item);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	for (ItemRenderInfos* item : layout) {
		delete item;
	}

	delete _painter;
	delete _writer;
	_painter = nullptr;
	_writer = nullptr;

	return status;
}


DocumentRenderer::RenderingStatus DocumentRenderer::render(QVector<ItemRenderInfos*> const& layout, RenderPluginManager const& pluginManager, QString const& filename) {

	_pluginManager = &pluginManager;

	if (layout.isEmpty()) {
		return RenderingStatus{MissingData, QObject::tr("Missing layout")};
	}

	if (_docTemplate == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid template")};
	}

	if (_writer != nullptr) {
		delete _writer;
	}

	_writer = new QPdfWriter(filename);
	_writer->setResolution(72);
	_writer->setTitle(_docTemplate->objectName());
	_writer->setPageMargins(QMarginsF(0,0,0,0));

	if (_painter != nullptr) {
		delete _painter;
	}

	_painter = new QPainter(_writer);
	_pagesToWrite = 0;
	_pagesWritten = 0;

	RenderingStatus status{Success, ""};

	for (ItemRenderInfos* item : layout) {

		RenderingStatus itemStatus = renderItem(*item);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	for (ItemRenderInfos* item : layout) {
		delete item;
	}

	delete _painter;
	delete _writer;
	_painter = nullptr;
	_writer = nullptr;

	return status;

}

void DocumentRenderer::ItemRenderInfos::translate(QPointF const& delta) {
	currentOrigin += delta;

	for (ItemRenderInfos* subItemInfos : subitemsRenderInfos) {
		if (subItemInfos != nullptr) {
			subItemInfos->translate(delta);
		}
	}
}

DocumentRenderer::RenderingStatus DocumentRenderer::layoutDocument(QVector<ItemRenderInfos*> & topLevel, DocumentDataInterface const* dataInterface) {

	if (_docTemplate == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid template")};
	}

	RenderingStatus status{Success, ""};

	for (DocumentItem* item : _docTemplate->subitems()) {

		DocumentValue val = dataInterface->getValue(item->dataKey());

		ItemRenderInfos* itemInfos = new ItemRenderInfos();
		itemInfos->item = item;
		itemInfos->itemValue = val;
		itemInfos->currentSize = item->initialSize();
		itemInfos->maxSize = item->maxSize();
		itemInfos->rendered = false;
		itemInfos->continuationIndex = QVariant();
		itemInfos->layoutStatus = Success;
		topLevel.push_back(itemInfos);

		RenderingStatus itemStatus = layoutItem(*topLevel.back(), nullptr, &topLevel);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutItem(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender, QVector<ItemRenderInfos*>* targetItemPool) {

	if (previousRender != nullptr) {
		if (previousRender->layoutStatus == Success) {

			if (previousRender->item != nullptr) {
				if (previousRender->item->overflowBehavior() != DocumentItem::OverflowBehavior::CopyOnNewPages) {
					goto rerender_error;
				}
			} else {
				goto rerender_error;
			}
		}
	}

	switch(itemInfos.item->getType()) {
	case DocumentItem::Type::Condition:
		return layoutCondition(itemInfos, previousRender);
	case DocumentItem::Type::Loop:
		return layoutLoop(itemInfos, previousRender);
	case DocumentItem::Type::Page:
		return layoutPage(itemInfos, previousRender, targetItemPool);
	case DocumentItem::Type::List:
		return layoutList(itemInfos, previousRender);
	case DocumentItem::Type::Frame:
		return layoutFrame(itemInfos, previousRender);
	case DocumentItem::Type::Text:
		return layoutText(itemInfos, previousRender);
	case DocumentItem::Type::Image:
		return layoutImage(itemInfos, previousRender);
	case DocumentItem::Type::Plugin:
		return RenderingStatus{OtherError, QObject::tr("Plugins not yet supported, block : %1").arg(itemInfos.item->objectName())};
	case DocumentItem::Type::Invalid:
		return RenderingStatus{OtherError, QObject::tr("Invalid block : %1").arg(itemInfos.item->objectName())};
	}

	return RenderingStatus{OtherError, QObject::tr("Unknown error for block : %1").arg(itemInfos.item->objectName())};

	rerender_error:
	return RenderingStatus{OtherError, QObject::tr("Requested to rerender an already rendered item!").arg(itemInfos.item->objectName())};

}

DocumentRenderer::RenderingStatus DocumentRenderer::layoutCondition(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel,
					QObject::tr("Invalid item requested!")};
	}

	if (!itemInfos.itemValue.hasMap()) {
		return RenderingStatus{MissingData,
					QObject::tr("Cannot read context map for condition : %1").arg(itemInfos.item->objectName())};
	}

	if (itemInfos.item->subitems().size() != 2) {
		return RenderingStatus{MissingModel,
					QObject::tr("Condition : %1, does not have two subitems, conditions should have exactly two subitems").arg(itemInfos.item->objectName())};
	}

	QString conditionKey = itemInfos.item->data();

	DocumentValue docdata = itemInfos.itemValue.getValue(conditionKey);

	QVariant conditionData;

	if (docdata.hasData()) {
		conditionData = docdata.getValue();
	}

	bool condition = conditionData.toBool();

	DocumentItem* target_item = itemInfos.item->subitems()[1];

	if (condition) {
		target_item = itemInfos.item->subitems()[0];
	}


	DocumentValue target_val = itemInfos.itemValue.getValue(target_item->dataKey());

	ItemRenderInfos* subItemInfos = new ItemRenderInfos();
	subItemInfos->item = target_item;
	subItemInfos->itemValue = target_val;
	subItemInfos->currentSize = target_item->initialSize();
	subItemInfos->maxSize = target_item->maxSize();
	subItemInfos->rendered = false;
	subItemInfos->continuationIndex = QVariant();
	subItemInfos->layoutStatus = Success;

	ItemRenderInfos* subPreviousRender = nullptr;

	if (previousRender != nullptr) {
		if (!previousRender->subitemsRenderInfos.isEmpty()) {
			if (previousRender->subitemsRenderInfos.first()->item == target_item) {
				subPreviousRender = previousRender->subitemsRenderInfos.first();
			}
		}
	}

	bool no_render_needed = false;

	if (subPreviousRender != nullptr) {
		if (subPreviousRender->layoutStatus == Success) {

			if (subPreviousRender->item != nullptr) {
				if (subPreviousRender->item->overflowBehavior() != DocumentItem::OverflowBehavior::CopyOnNewPages) {
					no_render_needed = true;
				}
			} else {
				no_render_needed = true;
			}
		}
	}

	if (no_render_needed) {
		delete subItemInfos;
		return RenderingStatus{Success};
	}

	itemInfos.subitemsRenderInfos.push_back(subItemInfos);

	return layoutItem(*subItemInfos, subPreviousRender, &itemInfos.subitemsRenderInfos);
}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutLoop(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (!itemInfos.itemValue.hasArray()) {
		return RenderingStatus{MissingData,
					QObject::tr("Cannot read context array for loop : %1").arg(itemInfos.item->objectName())};
	}

	if (itemInfos.item->subitems().size() != 1) {
		return RenderingStatus{MissingModel,
					QObject::tr("Loop : %1, does not have one subitem, loops should have exactly one subitem (the delegate)").arg(itemInfos.item->objectName())};
	}

	int nCopies = itemInfos.itemValue.arraySize();

	RenderContext oldContext = _renderContext;

	QSizeF renderSize(0,0);
	_renderContext = RenderContext{itemInfos.item->direction(),
			oldContext.origin + itemInfos.item->origin(),
			oldContext.region.boundedTo(itemInfos.item->initialSize())};

	//resize the context to the max size in layout direction
	if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
			itemInfos.item->direction() == DocumentItem::Top2Bottom) {

		_renderContext.region.setHeight(itemInfos.item->maxHeight());
		renderSize.setWidth(itemInfos.item->initialWidth());

	} else {
		_renderContext.region.setWidth(itemInfos.item->maxWidth());
		renderSize.setHeight(itemInfos.item->initialHeight());
	}

	QString message = "";

	int startsId = 0;
	itemInfos.layoutStatus = Success;

	if (previousRender != nullptr) {
		if (previousRender->continuationIndex.canConvert<int>()) {
			startsId = previousRender->continuationIndex.toInt();

			if (previousRender->subitemsRenderInfos.isEmpty()) {
				return RenderingStatus{OtherError,
							QObject::tr("Loop with empty non null previous render should not occur").arg(itemInfos.item->objectName())};
			}

			if (previousRender->subitemsRenderInfos.last()->layoutStatus != NotAllItemsRendered) {
				if (previousRender->subitemsRenderInfos.last()->layoutStatus != MissingSpace) {
					startsId++;
				}
			} else if (previousRender->subitemsRenderInfos.last()->layoutStatus == NotAllItemsRendered) {
				if (previousRender->subitemsRenderInfos.last()->item != nullptr) {
					if (previousRender->subitemsRenderInfos.last()->item->overflowBehavior() != DocumentItem::OverflowOnNewPage) {
						startsId++;
					}
				} else {
					startsId++;
				}
			}
		}
	}

	if (startsId >= nCopies) {
		itemInfos.layoutStatus = Success;
		return RenderingStatus{Success, "", renderSize};
	}

	for (int i = startsId; i < nCopies; i++) {

		ItemRenderInfos* subItemInfos = new ItemRenderInfos();
		subItemInfos->item = itemInfos.item->subitems()[0];
		subItemInfos->itemValue = itemInfos.itemValue.getValue(i);
		subItemInfos->currentSize = subItemInfos->item->initialSize();
		subItemInfos->maxSize = subItemInfos->item->maxSize();
		subItemInfos->rendered = false;
		subItemInfos->continuationIndex = QVariant();
		subItemInfos->layoutStatus = Success;

		itemInfos.subitemsRenderInfos.push_back(subItemInfos);

		ItemRenderInfos* previousInfos = nullptr;

		if (previousRender != nullptr) {
			if (i == startsId and previousRender->subitemsRenderInfos.last()->item == subItemInfos->item) {
				previousInfos = previousRender->subitemsRenderInfos.last();
			}
		}

		RenderingStatus layoutStatus = layoutItem(*subItemInfos, previousInfos, &itemInfos.subitemsRenderInfos);

		itemInfos.continuationIndex = i;

		if (layoutStatus.status == MissingSpace) {
			if (i == startsId) {
				itemInfos.layoutStatus = MissingSpace;
				message = layoutStatus.message +
						QString("\n Table: %1 missing space to render at least one item").arg(itemInfos.item->objectName());
			} else {
				subItemInfos->toRender = false;
				itemInfos.layoutStatus = NotAllItemsRendered;
				itemInfos.subitemsRenderInfos.removeLast();
				delete subItemInfos;
				itemInfos.continuationIndex = i-1;
			}
			break;
		} else if (layoutStatus.status == NotAllItemsRendered) {
			itemInfos.layoutStatus = NotAllItemsRendered;
			break;
		} else if (layoutStatus.status != Success) {
			itemInfos.layoutStatus = layoutStatus.status;
			break;
		}

		switch(_renderContext.direction) {
		case DocumentItem::Left2Right:
			_renderContext.origin.rx() += layoutStatus.renderSize.width();
			_renderContext.region.rwidth() -= layoutStatus.renderSize.width();
			renderSize.rwidth() += layoutStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), layoutStatus.renderSize.height());
			break;
		case DocumentItem::Right2Left:
			_renderContext.origin.rx() -= layoutStatus.renderSize.width();
			_renderContext.region.rwidth() -= layoutStatus.renderSize.width();
			renderSize.rwidth() += layoutStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), layoutStatus.renderSize.height());
			break;
		case DocumentItem::Top2Bottom:
			_renderContext.origin.ry() += layoutStatus.renderSize.height();
			_renderContext.region.rheight() -= layoutStatus.renderSize.height();
			renderSize.rwidth() = std::max(renderSize.width(), layoutStatus.renderSize.width());
			renderSize.rheight() += layoutStatus.renderSize.height();
			break;
		case DocumentItem::Bottom2Top:
			renderSize.rwidth() += std::max(renderSize.height(), layoutStatus.renderSize.height());
			renderSize.rheight() += layoutStatus.renderSize.height();
			break;
		}
	}

	//deal with remaining empty space.

	qreal remaining_space = 0;
	int nExpandable = 0;
	qreal pos_delta = 0;
	qreal expand_amount = 0;

	if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
			itemInfos.item->direction() == DocumentItem::Top2Bottom) {

		remaining_space = itemInfos.item->initialHeight() - renderSize.height();

	} else {

		remaining_space = itemInfos.item->initialWidth() - renderSize.width();
	}

	if (remaining_space <= 0) {
		goto end_layout;
	}

	if (renderSize.height() < itemInfos.item->initialHeight()) {
		renderSize.rheight() = itemInfos.item->initialHeight();
	}

	if (renderSize.width() < itemInfos.item->initialWidth()) {
		renderSize.rwidth() = itemInfos.item->initialWidth();
	}

	for (int i = 0; i < itemInfos.subitemsRenderInfos.size(); i++) {
		if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() != DocumentItem::LayoutExpandBehavior::NotExpand) {
			nExpandable++;
		}
	}

	if (nExpandable <= 0) {
		goto end_layout;
	}

	expand_amount = remaining_space/nExpandable;

	for (int i = 0; i < itemInfos.subitemsRenderInfos.size(); i++) {

		if (itemInfos.item->direction() == DocumentItem::Top2Bottom) {

			itemInfos.subitemsRenderInfos[i]->currentOrigin.ry() += pos_delta;

		} else if (itemInfos.item->direction() == DocumentItem::Bottom2Top) {

			itemInfos.subitemsRenderInfos[i]->currentOrigin.ry() -= pos_delta;

		} else if (itemInfos.item->direction() == DocumentItem::Left2Right) {

			itemInfos.subitemsRenderInfos[i]->currentOrigin.rx() += pos_delta;

		} else {
			itemInfos.subitemsRenderInfos[i]->currentOrigin.rx() -= pos_delta;
		}

		if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() != DocumentItem::LayoutExpandBehavior::NotExpand) {

			if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
					itemInfos.item->direction() == DocumentItem::Top2Bottom) {

				if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: Expand) {
					itemInfos.subitemsRenderInfos[i]->currentSize.rheight() += expand_amount;

					if (itemInfos.item->direction() == DocumentItem::Bottom2Top ) {
						itemInfos.subitemsRenderInfos[i]->currentOrigin.ry() -= expand_amount;
					}

				} else if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: ExpandMargins) {

					qreal scale = (itemInfos.item->direction() == DocumentItem::Bottom2Top ) ? -1 : 1;

					auto marginExpandBehavior = itemInfos.subitemsRenderInfos[i]->item->marginsExpandBehavior();
					itemInfos.subitemsRenderInfos[i]->currentOrigin.ry() +=
							scale * ((marginExpandBehavior == DocumentItem::ExpandBefore) ? expand_amount :
																				   ((marginExpandBehavior == DocumentItem::ExpandBoth) ? expand_amount/2 : 0));
				}

			} else {

				if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: Expand) {
					itemInfos.subitemsRenderInfos[i]->currentSize.rwidth() += expand_amount;

					if (itemInfos.item->direction() == DocumentItem::Right2Left ) {
						itemInfos.subitemsRenderInfos[i]->currentOrigin.rx() -= expand_amount;
					}

				} else if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: ExpandMargins) {

					qreal scale = (itemInfos.item->direction() == DocumentItem::Right2Left ) ? -1 : 1;

					auto marginExpandBehavior = itemInfos.subitemsRenderInfos[i]->item->marginsExpandBehavior();
					itemInfos.subitemsRenderInfos[i]->currentOrigin.rx() +=
							scale * ((marginExpandBehavior == DocumentItem::ExpandBefore) ? expand_amount :
																				   ((marginExpandBehavior == DocumentItem::ExpandBoth) ? expand_amount/2 : 0));
				}
			}

			pos_delta += expand_amount;

		}
	}

	end_layout:

	_renderContext = oldContext;

	return RenderingStatus{itemInfos.layoutStatus, message, renderSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutPage(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender, QVector<ItemRenderInfos*>* targetItemPool) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	_renderContext = RenderContext{itemInfos.item->direction(), itemInfos.item->origin(), itemInfos.item->initialSize()};

	RenderingStatus status{Success, ""};

	int nItems = itemInfos.item->subitems().size();

	bool hasMoreToRender = false;
	bool isFirst = true;

	ItemRenderInfos* currentPageInfos = &itemInfos;
	ItemRenderInfos* previousPageInfos = previousRender;

	do {

		hasMoreToRender = false;

		for (int i = 0; i < nItems; i++) {

			if (!isFirst and itemInfos.item->subitems()[i]->overflowBehavior() == DocumentItem::DrawFirstInstanceOnly) {

				currentPageInfos->subitemsRenderInfos.push_back(nullptr);
				continue; //skip items configured to draw first instance only.
			}

			ItemRenderInfos* subItemInfos = new ItemRenderInfos();
			subItemInfos->item = itemInfos.item->subitems()[i];
			subItemInfos->itemValue = itemInfos.itemValue.getValue(subItemInfos->item->dataKey());
			subItemInfos->currentSize = subItemInfos->item->initialSize();
			subItemInfos->maxSize = subItemInfos->item->maxSize();
			subItemInfos->rendered = false;
			subItemInfos->continuationIndex = QVariant();
			subItemInfos->layoutStatus = Success;

			ItemRenderInfos* previousItemRenderInfos = nullptr;

			if (previousPageInfos != nullptr) {
				if (previousPageInfos->subitemsRenderInfos.size() > i) {
					previousItemRenderInfos = previousPageInfos->subitemsRenderInfos[i];
				}
			}

			if (previousItemRenderInfos != nullptr) {
				if (previousItemRenderInfos->layoutStatus == Success) {

					if (previousItemRenderInfos->item != nullptr) {
						if (previousItemRenderInfos->item->overflowBehavior() != DocumentItem::OverflowBehavior::CopyOnNewPages) {
							delete subItemInfos;
							currentPageInfos->subitemsRenderInfos.push_back(nullptr);
							continue;
						}
					} else {
						delete subItemInfos;
						currentPageInfos->subitemsRenderInfos.push_back(nullptr);
						continue;
					}
				}
			}

			currentPageInfos->subitemsRenderInfos.push_back(subItemInfos);

			RenderingStatus itemStatus = layoutItem(*subItemInfos, previousItemRenderInfos);

			if (itemStatus.status == NotAllItemsRendered) {
				if (subItemInfos->item->overflowBehavior() == DocumentItem::OverflowOnNewPage) {
					hasMoreToRender = true;
				} else {
					status.status = MissingSpace;
					if (!status.message.isEmpty()) {
						status.message += "\n";
					}
					status.message += "Missing space for non-overflowing item";
				}
			} else if (itemStatus.status != Success) {
				status.status = itemStatus.status;
				if (!status.message.isEmpty()) {
					status.message += "\n";
				}
				status.message += itemStatus.message;
			}
		}

		_pagesToWrite++;
		isFirst = false;

		if (targetItemPool == nullptr) {
			break; //impossible to add more pages if no targetItemPool provided
		}

		if (hasMoreToRender) {
			previousPageInfos = currentPageInfos;
			currentPageInfos = new ItemRenderInfos();
			currentPageInfos->item = itemInfos.item;
			currentPageInfos->itemValue = itemInfos.itemValue;
			currentPageInfos->currentSize = itemInfos.currentSize;
			currentPageInfos->maxSize = itemInfos.maxSize;
			currentPageInfos->rendered = false;
			currentPageInfos->continuationIndex = QVariant();
			currentPageInfos->layoutStatus = Success;

			targetItemPool->push_back(currentPageInfos);
		}

	} while (hasMoreToRender);

	return status;
}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutList(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	int nItems = itemInfos.item->subitems().size();

	RenderContext oldContext = _renderContext;

	QSizeF renderSize(0,0);
	_renderContext = RenderContext{itemInfos.item->direction(),
			oldContext.origin + itemInfos.item->origin(),
			oldContext.region.boundedTo(itemInfos.item->initialSize())};

	//resize the context to the max size in layout direction
	if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
			itemInfos.item->direction() == DocumentItem::Top2Bottom) {

		_renderContext.region.setHeight(itemInfos.item->maxHeight());
		renderSize.setWidth(itemInfos.item->initialWidth());

	} else {
		_renderContext.region.setWidth(itemInfos.item->maxWidth());
		renderSize.setHeight(itemInfos.item->initialHeight());
	}

	QString message = "";

	int startsId = 0;

	if (previousRender != nullptr) {
		if (previousRender->continuationIndex.canConvert<int>()) {
			startsId = previousRender->continuationIndex.toInt();

			if (previousRender->subitemsRenderInfos.isEmpty()) {
				return RenderingStatus{OtherError,
							QObject::tr("List with empty non null previous render should not occur").arg(itemInfos.item->objectName())};
			}

			if (previousRender->subitemsRenderInfos.last()->layoutStatus != NotAllItemsRendered) {
				startsId++;
			} else if (previousRender->subitemsRenderInfos.last()->layoutStatus == NotAllItemsRendered) {
				if (previousRender->subitemsRenderInfos.last()->item != nullptr) {
					if (previousRender->subitemsRenderInfos.last()->item->overflowBehavior() != DocumentItem::OverflowOnNewPage) {
						startsId++;
					}
				} else {
					startsId++;
				}
			}
		}
	}

	if (startsId >= nItems) {
		itemInfos.layoutStatus = Success;
		return RenderingStatus{Success, "", renderSize};
	}

	for (int i = startsId; i < nItems; i++) {

		ItemRenderInfos* subItemInfos = new ItemRenderInfos();
		subItemInfos->item = itemInfos.item->subitems()[i];

		if (itemInfos.itemValue.hasArray()) { //in case an array was provided, use the index
			subItemInfos->itemValue = itemInfos.itemValue.getValue(i);
		} else { //else, use the datakey
			subItemInfos->itemValue = itemInfos.itemValue.getValue(subItemInfos->item->dataKey());
		}

		subItemInfos->currentSize = subItemInfos->item->initialSize();
		subItemInfos->maxSize = subItemInfos->item->maxSize();
		subItemInfos->rendered = false;
		subItemInfos->continuationIndex = QVariant();
		subItemInfos->layoutStatus = Success;

		itemInfos.subitemsRenderInfos.push_back(subItemInfos);

		ItemRenderInfos* previousInfos = nullptr;

		if (previousRender != nullptr) {
			if (i == startsId and previousRender->subitemsRenderInfos.last()->item == subItemInfos->item) {
				previousInfos = previousRender->subitemsRenderInfos.last();
			}
		}

		RenderingStatus layoutStatus = layoutItem(*subItemInfos, previousInfos);

		itemInfos.continuationIndex = i;

		if (layoutStatus.status == MissingSpace) {
			if (i == startsId) {
				itemInfos.layoutStatus = MissingSpace;
				message = layoutStatus.message +
						QString("\n Table: %1 missing space to render at least one item").arg(itemInfos.item->objectName());
			} else {
				itemInfos.layoutStatus = NotAllItemsRendered;
				itemInfos.subitemsRenderInfos.removeLast();
				delete subItemInfos;
				itemInfos.continuationIndex = i-1;
			}
			break;
		} else if (layoutStatus.status == NotAllItemsRendered) {
			itemInfos.layoutStatus = NotAllItemsRendered;
			break;
		} else if (layoutStatus.status != Success) {
			itemInfos.layoutStatus = layoutStatus.status;
			break;
		}

		switch(_renderContext.direction) {
		case DocumentItem::Left2Right:
			_renderContext.origin.rx() += layoutStatus.renderSize.width();
			_renderContext.region.rwidth() -= layoutStatus.renderSize.width();
			renderSize.rwidth() += layoutStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), layoutStatus.renderSize.height());
			break;
		case DocumentItem::Right2Left:
			_renderContext.region.rwidth() -= layoutStatus.renderSize.width();
			renderSize.rwidth() += layoutStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), layoutStatus.renderSize.height());
			break;
		case DocumentItem::Top2Bottom:
			_renderContext.origin.ry() += layoutStatus.renderSize.height();
			_renderContext.region.rheight() -= layoutStatus.renderSize.height();
			renderSize.rwidth() = std::max(renderSize.width(), layoutStatus.renderSize.width());
			renderSize.rheight() += layoutStatus.renderSize.height();
			break;
		case DocumentItem::Bottom2Top:
			_renderContext.region.rheight() -= layoutStatus.renderSize.height();
			renderSize.rwidth() = std::max(renderSize.height(), layoutStatus.renderSize.height());
			renderSize.rheight() += layoutStatus.renderSize.height();
			break;
		}
	}

	//deal with remaining empty space.

	qreal remaining_space = 0;
	int nExpandable = 0;
	qreal pos_delta = 0;
	qreal expand_amount = 0;

	if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
			itemInfos.item->direction() == DocumentItem::Top2Bottom) {

		remaining_space = itemInfos.item->initialHeight() - renderSize.height();

	} else {

		remaining_space = itemInfos.item->initialWidth() - renderSize.width();
	}

	if (remaining_space <= 0) {
		goto end_layout;
	}

	if (renderSize.height() < itemInfos.item->initialHeight()) {
		renderSize.rheight() = itemInfos.item->initialHeight();
	}

	if (renderSize.width() < itemInfos.item->initialWidth()) {
		renderSize.rwidth() = itemInfos.item->initialWidth();
	}

	for (int i = 0; i < itemInfos.subitemsRenderInfos.size(); i++) {
		if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() != DocumentItem::LayoutExpandBehavior::NotExpand) {
			nExpandable++;
		}
	}

	if (nExpandable <= 0) {
		goto end_layout;
	}

	expand_amount = remaining_space/nExpandable;

	for (int i = 0; i < itemInfos.subitemsRenderInfos.size(); i++) {

		if (itemInfos.item->direction() == DocumentItem::Top2Bottom) {

			itemInfos.subitemsRenderInfos[i]->translate(QPointF(0,pos_delta));

		} else if (itemInfos.item->direction() == DocumentItem::Bottom2Top) {

			itemInfos.subitemsRenderInfos[i]->translate(QPointF(0,-pos_delta));

		} else if (itemInfos.item->direction() == DocumentItem::Left2Right) {

			itemInfos.subitemsRenderInfos[i]->translate(QPointF(pos_delta,0));

		} else {

			itemInfos.subitemsRenderInfos[i]->translate(QPointF(-pos_delta,0));

		}

		if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() != DocumentItem::LayoutExpandBehavior::NotExpand) {

			if (itemInfos.item->direction() == DocumentItem::Bottom2Top or
					itemInfos.item->direction() == DocumentItem::Top2Bottom) {

				if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: Expand) {
					itemInfos.subitemsRenderInfos[i]->currentSize.rheight() += expand_amount;

					if (itemInfos.item->direction() == DocumentItem::Bottom2Top ) {
						itemInfos.subitemsRenderInfos[i]->translate(QPointF(0,-expand_amount));
					}

				} else if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: ExpandMargins) {

					qreal scale = (itemInfos.item->direction() == DocumentItem::Bottom2Top ) ? -1 : 1;

					auto marginExpandBehavior = itemInfos.subitemsRenderInfos[i]->item->marginsExpandBehavior();
					qreal dy =
							scale * ((marginExpandBehavior == DocumentItem::ExpandBefore) ? expand_amount :
																				   ((marginExpandBehavior == DocumentItem::ExpandBoth) ? expand_amount/2 : 0));

					itemInfos.subitemsRenderInfos[i]->translate(QPointF(0,dy));
				}

			} else {

				if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: Expand) {
					itemInfos.subitemsRenderInfos[i]->currentSize.rwidth() += expand_amount;

					if (itemInfos.item->direction() == DocumentItem::Right2Left ) {
						itemInfos.subitemsRenderInfos[i]->translate(QPointF(-expand_amount,0));
					}

				} else if (itemInfos.subitemsRenderInfos[i]->item->layoutExpandBehavior() == DocumentItem::LayoutExpandBehavior:: ExpandMargins) {

					qreal scale = (itemInfos.item->direction() == DocumentItem::Right2Left ) ? -1 : 1;

					auto marginExpandBehavior = itemInfos.subitemsRenderInfos[i]->item->marginsExpandBehavior();
					qreal dx =
							scale * ((marginExpandBehavior == DocumentItem::ExpandBefore) ? expand_amount :
																				   ((marginExpandBehavior == DocumentItem::ExpandBoth) ? expand_amount/2 : 0));

					itemInfos.subitemsRenderInfos[i]->translate(QPointF(dx,0));
				}
			}

			pos_delta += expand_amount;

		}
	}

	end_layout:

	_renderContext = oldContext;

	return RenderingStatus{itemInfos.layoutStatus, message, renderSize};

}

DocumentRenderer::RenderingStatus DocumentRenderer::layoutFrame(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QSizeF itemInitialSize = itemInfos.item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
			itemInitialSize.height() > _renderContext.region.height()) {
		return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Frame: %1").arg(itemInfos.item->objectName())};
	}

	int nItems = itemInfos.item->subitems().size();

	RenderContext oldContext = _renderContext;

	QPointF origin = oldContext.origin + itemInfos.item->origin();

	if (oldContext.direction == DocumentItem::Right2Left) {
		origin.rx() = oldContext.origin.x() - itemInfos.item->origin().x();
	}

	if (oldContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = oldContext.origin.y() - itemInfos.item->origin().y();
	}

	itemInfos.currentOrigin = origin;

	QSizeF renderSize(itemInfos.item->initialSize());
	_renderContext = RenderContext{itemInfos.item->direction(),
			origin,
			itemInfos.item->initialSize()};

	RenderingStatus status{Success, ""};

	for (int i = 0; i < nItems; i++) {

		ItemRenderInfos* subItemInfos = new ItemRenderInfos();
		subItemInfos->item = itemInfos.item->subitems()[i];
		subItemInfos->itemValue = itemInfos.itemValue.getValue(subItemInfos->item->dataKey());
		subItemInfos->currentSize = subItemInfos->item->initialSize();
		subItemInfos->maxSize = subItemInfos->item->maxSize();
		subItemInfos->rendered = false;
		subItemInfos->continuationIndex = QVariant();
		subItemInfos->layoutStatus = Success;

		ItemRenderInfos* previousItemRenderInfos = nullptr;

		if (previousRender != nullptr) {
			if (previousRender->subitemsRenderInfos.size() > i) {
				previousItemRenderInfos = previousRender->subitemsRenderInfos[i];
			}
		}

		if (previousItemRenderInfos != nullptr) {
			if (previousItemRenderInfos->layoutStatus == Success) {

				if (previousItemRenderInfos->item != nullptr) {
					if (previousItemRenderInfos->item->overflowBehavior() != DocumentItem::OverflowBehavior::CopyOnNewPages) {
						delete subItemInfos;
						continue;
					}
				} else {
					delete subItemInfos;
					continue;
				}
			}
		}

		itemInfos.subitemsRenderInfos.push_back(subItemInfos);

		RenderingStatus itemStatus = layoutItem(*subItemInfos, previousItemRenderInfos);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	status.renderSize = renderSize;

	_renderContext = oldContext;

	return status;
}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutText(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (_painter == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid painter used for layout!")};
	}

	QSizeF itemInitialSize = itemInfos.item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
			itemInitialSize.height() > _renderContext.region.height()) {

		itemInfos.layoutStatus = MissingSpace;
		return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Text: %1").arg(itemInfos.item->objectName())};
	}

	QVariant variant = itemInfos.itemValue.getValue();
	QString text;

	if (variant.isValid()) {
		text = variant.toString();
	} else {
		text = itemInfos.item->data();
	}

	QPointF origin = _renderContext.origin + itemInfos.item->origin();

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = _renderContext.origin.x() - itemInfos.item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = _renderContext.origin.y() - itemInfos.item->origin().y();
	}

	itemInfos.currentOrigin = origin;

	QSizeF renderSize(itemInfos.item->initialSize());

	QFont font("serif", 12);
	font.setFamily(itemInfos.item->fontName());
	font.setPointSizeF(itemInfos.item->fontSize());
	QFontMetrics fontMetric(font);

	int flags = Qt::AlignLeft;

	switch (itemInfos.item->textAlign()) {
	case DocumentItem::TextAlign::AlignLeft:
		flags = Qt::AlignLeft;
		break;
	case DocumentItem::TextAlign::AlignRight:
		flags = Qt::AlignRight;
		break;
	case DocumentItem::TextAlign::AlignCenter:
		flags = Qt::AlignHCenter;
		break;
	case DocumentItem::TextAlign::AlignJustify:
		flags = Qt::AlignJustify;
		break;
	}

	QRectF rectangle = QRectF(origin, renderSize);
	QRectF boundingRect = fontMetric.boundingRect(rectangle.toRect(), flags,  text);

	RenderingStatus status{Success, "", renderSize};

	if (boundingRect.width() > rectangle.width() or boundingRect.height() > rectangle.height()) {
		//in can the initial size is not enough

		QSizeF maxRenderSize(itemInfos.item->maxSize());
		QRectF rectangle = QRectF(origin, maxRenderSize);
		boundingRect = fontMetric.boundingRect(rectangle.toRect(), flags,  text);

		if (boundingRect.width() > rectangle.width() or boundingRect.height() > rectangle.height()) {
			status.status = MissingSpace;
			status.message = QObject::tr("Text from text block %1 overflow").arg(itemInfos.item->objectName());
		} else {
			status.renderSize = boundingRect.size();
		}
	}

	itemInfos.layoutStatus = status.status;
	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutImage(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender){

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QSizeF itemInitialSize = itemInfos.item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
		itemInitialSize.height() > _renderContext.region.height()) {
		return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Image: %1").arg(itemInfos.item->objectName())};
	}

	QVariant variant = itemInfos.itemValue.getValue();
	QImage image;

	if (variant.isValid()) {
		if (variant.canConvert<QImage>()) {
			image = qvariant_cast<QImage>(variant);
		} else if (variant.canConvert<QString>()) {
			image = QImage(variant.toString());
		}
	} else {
		image = QImage(itemInfos.item->data());
	}

	if (image.isNull()) {
		return RenderingStatus{MissingData, QObject::tr("Cannot load data for Image: %1").arg(itemInfos.item->objectName())};
	}

	QPointF origin = _renderContext.origin + itemInfos.item->origin();

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = _renderContext.origin.x() - itemInfos.item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = _renderContext.origin.y() - itemInfos.item->origin().y();
	}

	itemInfos.currentOrigin = origin;

	QSizeF renderSize(itemInfos.item->initialSize());

	RenderingStatus status{Success, "", renderSize};

	return status;
}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutPlugin(ItemRenderInfos& itemInfos, ItemRenderInfos* previousRender) {

	if (_pluginManager == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Missing plugin manager")};
	}

	RenderPlugin const* plugin = _pluginManager->getPlugin(itemInfos.item->data());

	if (plugin == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Requested missing plugin")};
	}

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QSizeF itemInitialSize = itemInfos.item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
		itemInitialSize.height() > _renderContext.region.height()) {
		return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Image: %1").arg(itemInfos.item->objectName())};
	}

	if (itemInitialSize.width() < itemInfos.maxSize.width()) {
		itemInitialSize.rwidth() = itemInfos.maxSize.width();
	}

	if (itemInitialSize.height() < itemInfos.maxSize.height()) {
		itemInitialSize.rheight() = itemInfos.maxSize.height();
	}

	itemInitialSize = itemInitialSize.boundedTo(_renderContext.region);

	QPointF origin = _renderContext.origin + itemInfos.item->origin();

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = _renderContext.origin.x() - itemInfos.item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = _renderContext.origin.y() - itemInfos.item->origin().y();
	}

	QRectF requiredRegion = plugin->getMinimalSpace(QRectF(origin, itemInitialSize), itemInfos.itemValue);

	itemInfos.currentOrigin = requiredRegion.topLeft();
	itemInfos.currentSize = requiredRegion.size();
	itemInfos.layoutStatus = Success;

	return{Success};
}


DocumentRenderer::RenderingStatus DocumentRenderer::renderItem(ItemRenderInfos& itemInfos) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (!itemInfos.toRender) {
		return RenderingStatus{Success}; //just skip rendering the item.
	}

	switch(itemInfos.item->getType()) {
	case DocumentItem::Type::Condition:
		return renderCondition(itemInfos);
	case DocumentItem::Type::Loop:
		return renderLoop(itemInfos);
	case DocumentItem::Type::Page:
		return renderPage(itemInfos);
	case DocumentItem::Type::List:
		return renderList(itemInfos);
	case DocumentItem::Type::Frame:
		return renderFrame(itemInfos);
	case DocumentItem::Type::Text:
		return renderText(itemInfos);
	case DocumentItem::Type::Image:
		return renderImage(itemInfos);
	case DocumentItem::Type::Plugin:
		return renderPlugin(itemInfos);
	case DocumentItem::Type::Invalid:
		return RenderingStatus{OtherError, QObject::tr("Invalid block : %1").arg(itemInfos.item->objectName())};
	}

	return RenderingStatus{OtherError, QObject::tr("Unknown error for block : %1").arg(itemInfos.item->objectName())};
}


DocumentRenderer::RenderingStatus DocumentRenderer::renderCondition(ItemRenderInfos& itemInfos) {

	for (ItemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
		if (subitemInfos == nullptr) {
			continue;
		}
		RenderingStatus status = renderItem(*subitemInfos);

		if (status.status != Success) {
			return status;
		}
	}

	return RenderingStatus{Success, "", itemInfos.currentSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderLoop(ItemRenderInfos& itemInfos) {

	RenderingStatus status{Success, ""};

	for (ItemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
		if (subitemInfos == nullptr) {
			continue;
		}
		RenderingStatus itemStatus = renderItem(*subitemInfos);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderPage(ItemRenderInfos& itemInfos) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QPageSize pageSize(itemInfos.currentSize, QPageSize::Unit::Point);
	QPageLayout layout;
	layout.setPageSize(pageSize);

	//ensure painting start on a new page straight after setting the page layout.
	if (_pagesWritten > 0) {
		_writer->newPage(); //create a new page in the writer.
	}

	RenderingStatus status{Success, ""};

	for (ItemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
		if (subitemInfos == nullptr) {
			continue;
		}
		RenderingStatus itemStatus = renderItem(*subitemInfos);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	_pagesWritten++;

	return status;


}
DocumentRenderer::RenderingStatus DocumentRenderer::renderList(ItemRenderInfos& itemInfos) {

	RenderingStatus status{Success, ""};

	for (ItemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
		if (subitemInfos == nullptr) {
			continue;
		}
		RenderingStatus itemStatus = renderItem(*subitemInfos);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderFrame(ItemRenderInfos& itemInfos) {

	RenderingStatus status{Success, ""};

	QRectF rect(itemInfos.currentOrigin, itemInfos.currentSize);

	if (itemInfos.item->fillColor().isValid()) {
		_painter->fillRect(rect, itemInfos.item->fillColor());
	}

	if (itemInfos.item->borderColor().isValid() and itemInfos.item->borderWidth() > 0) {

		QPen borderPen;
		borderPen.setColor(itemInfos.item->borderColor());
		borderPen.setWidthF(itemInfos.item->borderWidth());
		borderPen.setStyle(Qt::SolidLine);
		borderPen.setJoinStyle(Qt::MiterJoin);

		QPen oldPen = _painter->pen();
		_painter->setPen(borderPen);

		_painter->drawRect(rect);
		_painter->setPen(oldPen);
	}

	for (ItemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
		if (subitemInfos == nullptr) {
			continue;
		}
		RenderingStatus itemStatus = renderItem(*subitemInfos);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	return status;


}
DocumentRenderer::RenderingStatus DocumentRenderer::renderText(ItemRenderInfos& itemInfos) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (_painter == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid painter used for rendering!")};
	}

	QSizeF itemSize = itemInfos.currentSize;

	QVariant variant = itemInfos.itemValue.getValue();
	QString text;

	if (variant.isValid()) {
		text = variant.toString();
	} else {
		text = itemInfos.item->data();
	}

	QPointF origin = itemInfos.currentOrigin;

	QSizeF renderSize = itemInfos.currentSize;

	QFont font("serif", 12);
	font.setFamily(itemInfos.item->fontName());
	font.setPointSizeF(itemInfos.item->fontSize());
	_painter->setFont(font);

	QRectF rectangle = QRectF(origin, renderSize);
	QRectF boundingRect;
	_painter->drawText(rectangle, 0, text, &boundingRect);

	RenderingStatus status{Success, "", boundingRect.size()};

	if (boundingRect.width() > rectangle.width() or boundingRect.height() > rectangle.height()) {
		status.status = MissingSpace;
		status.message = QObject::tr("Text from text block %1 overflow").arg(itemInfos.item->objectName());
	}

	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderImage(ItemRenderInfos& itemInfos) {

	if (itemInfos.item == nullptr) {
        return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
    }

    if (_painter == nullptr) {
        return RenderingStatus{OtherError, QObject::tr("Invalid painter used for rendering!")};
    }

	QVariant variant = itemInfos.itemValue.getValue();
    QImage image;

    if (variant.isValid()) {
        if (variant.canConvert<QImage>()) {
            image = qvariant_cast<QImage>(variant);
        } else if (variant.canConvert<QString>()) {
            image = QImage(variant.toString());
        }
    } else {
		image = QImage(itemInfos.item->data());
    }

    if (image.isNull()) {
		return RenderingStatus{MissingData, QObject::tr("Not enough space to render Image: %1").arg(itemInfos.item->objectName())};
    }

	QPointF origin = itemInfos.currentOrigin;

	QSizeF renderSize = itemInfos.currentSize;

	QRectF rectangle = QRectF(origin, renderSize);
    _painter->drawImage(rectangle, image);

    RenderingStatus status{Success, "", rectangle.size()};

    return status;

}

DocumentRenderer::RenderingStatus DocumentRenderer::renderPlugin(ItemRenderInfos& itemInfos) {

	if (_pluginManager == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Missing plugin manager")};
	}

	RenderPlugin const* plugin = _pluginManager->getPlugin(itemInfos.item->data());

	if (plugin == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Requested missing plugin")};
	}

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	return plugin->renderItem(QRectF(itemInfos.currentOrigin, itemInfos.currentSize), *_painter, itemInfos.itemValue);

}

} // namespace AutoQuill
