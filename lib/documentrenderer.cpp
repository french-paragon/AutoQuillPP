#include "documentrenderer.h"

#include <QObject>

#include <QPdfWriter>
#include <QPainter>

#include "documenttemplate.h"
#include "documentitem.h"
#include "documentdatainterface.h"

DocumentRenderer::DocumentRenderer(DocumentTemplate* docTemplate) :
	_docTemplate(docTemplate),
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

DocumentRenderer::RenderingStatus DocumentRenderer::render(DocumentDataInterface* dataInterface, QString const& filename) {

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

	if (_painter != nullptr) {
		delete _painter;
	}

	_painter = new QPainter(_writer);
	_pagesToWrite = 0;
	_pagesWritten = 0;

	RenderingStatus status{Success, ""};

	QVector<itemRenderInfos*> layout;

	RenderingStatus layoutStatus = layoutDocument(layout, dataInterface);

	if (layoutStatus.status != Success) {
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

	for (itemRenderInfos* item : layout) {

		RenderingStatus itemStatus = renderItem(*item);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		}
	}

	for (itemRenderInfos* item : layout) {
		delete item;
	}

	delete _painter;
	delete _writer;
	_painter = nullptr;
	_writer = nullptr;

	return status;
}

DocumentRenderer::RenderingStatus DocumentRenderer::layoutDocument(QVector<itemRenderInfos*> & topLevel, DocumentDataInterface* dataInterface) {

	if (_docTemplate == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid template")};
	}

	RenderingStatus status{Success, ""};

	for (DocumentItem* item : _docTemplate->subitems()) {

		DocumentValue val = dataInterface->getValue(item->dataKey());

		itemRenderInfos* itemInfos = new itemRenderInfos();
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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutItem(itemRenderInfos& itemInfos, itemRenderInfos* previousRender, QVector<itemRenderInfos*>* targetItemPool) {

	if (previousRender != nullptr) {
		if (previousRender->renderStatus == Success) {

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

DocumentRenderer::RenderingStatus DocumentRenderer::layoutCondition(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

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

	itemRenderInfos* subItemInfos = new itemRenderInfos();
	subItemInfos->item = target_item;
	subItemInfos->itemValue = target_val;
	subItemInfos->currentSize = target_item->initialSize();
	subItemInfos->maxSize = target_item->maxSize();
	subItemInfos->rendered = false;
	subItemInfos->continuationIndex = QVariant();
	subItemInfos->layoutStatus = Success;

	itemRenderInfos* subPreviousRender = nullptr;

	if (previousRender != nullptr) {
		if (!previousRender->subitemsRenderInfos.isEmpty()) {
			if (previousRender->subitemsRenderInfos.first()->item == target_item) {
				subPreviousRender = previousRender->subitemsRenderInfos.first();
			}
		}
	}

	bool no_render_needed = false;

	if (subPreviousRender != nullptr) {
		if (subPreviousRender->renderStatus == Success) {

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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutLoop(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

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

	QString message = "";

	int startsId = 0;
	itemInfos.renderStatus = Success;

	if (previousRender != nullptr) {
		if (previousRender->continuationIndex.canConvert<int>()) {
			startsId = previousRender->continuationIndex.toInt();

			if (previousRender->subitemsRenderInfos.isEmpty()) {
				return RenderingStatus{OtherError,
							QObject::tr("Loop with empty non null previous render should not occur").arg(itemInfos.item->objectName())};
			}

			if (previousRender->subitemsRenderInfos.last()->renderStatus != NotAllItemsRendered) {
				if (previousRender->subitemsRenderInfos.last()->renderStatus != MissingSpace) {
					startsId++;
				}
			} else if (previousRender->subitemsRenderInfos.last()->renderStatus == NotAllItemsRendered) {
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
		itemInfos.renderStatus = Success;
		return RenderingStatus{Success, "", renderSize};
	}

	for (int i = startsId; i < nCopies; i++) {

		itemRenderInfos* subItemInfos = new itemRenderInfos();
		subItemInfos->item = itemInfos.item->subitems()[0];
		subItemInfos->itemValue = itemInfos.itemValue.getValue(i);
		subItemInfos->currentSize = subItemInfos->item->initialSize();
		subItemInfos->maxSize = subItemInfos->item->maxSize();
		subItemInfos->rendered = false;
		subItemInfos->continuationIndex = QVariant();
		subItemInfos->layoutStatus = Success;

		itemInfos.subitemsRenderInfos.push_back(subItemInfos);

		itemRenderInfos* previousInfos = nullptr;

		if (previousRender != nullptr) {
			if (i == startsId and previousRender->subitemsRenderInfos.last()->item == subItemInfos->item) {
				previousInfos = previousRender->subitemsRenderInfos.last();
			}
		}

		RenderingStatus layoutStatus = layoutItem(*subItemInfos, previousInfos, &itemInfos.subitemsRenderInfos);

		itemInfos.continuationIndex = i;

		if (layoutStatus.status == MissingSpace) {
			if (i == startsId) {
				itemInfos.renderStatus = MissingSpace;
				message = layoutStatus.message +
						QString("\n Table: %1 missing space to render at least one item").arg(itemInfos.item->objectName());
			} else {
				subItemInfos->toRender = false;
				itemInfos.renderStatus = NotAllItemsRendered;
			}
			break;
		} else if (layoutStatus.status == NotAllItemsRendered) {
			itemInfos.renderStatus = NotAllItemsRendered;
			break;
		} else if (layoutStatus.status != Success) {
			itemInfos.renderStatus = layoutStatus.status;
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

	_renderContext = oldContext;

	return RenderingStatus{itemInfos.renderStatus, message, renderSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutPage(itemRenderInfos& itemInfos, itemRenderInfos* previousRender, QVector<itemRenderInfos*>* targetItemPool) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	_renderContext = RenderContext{itemInfos.item->direction(), itemInfos.item->origin(), itemInfos.item->initialSize()};

	RenderingStatus status{Success, ""};

	int nItems = itemInfos.item->subitems().size();

	bool hasMoreToRender = false;
	bool isFirst = true;

	itemRenderInfos* currentPageInfos = &itemInfos;
	itemRenderInfos* previousPageInfos = previousRender;

	do {

		hasMoreToRender = false;

		for (int i = 0; i < nItems; i++) {

			if (!isFirst and itemInfos.item->subitems()[i]->overflowBehavior() == DocumentItem::DrawFirstInstanceOnly) {

				currentPageInfos->subitemsRenderInfos.push_back(nullptr);
				continue; //skip items configured to draw first instance only.
			}

			itemRenderInfos* subItemInfos = new itemRenderInfos();
			subItemInfos->item = itemInfos.item->subitems()[i];
			subItemInfos->itemValue = itemInfos.itemValue.getValue(subItemInfos->item->dataKey());
			subItemInfos->currentSize = subItemInfos->item->initialSize();
			subItemInfos->maxSize = subItemInfos->item->maxSize();
			subItemInfos->rendered = false;
			subItemInfos->continuationIndex = QVariant();
			subItemInfos->layoutStatus = Success;

			itemRenderInfos* previousItemRenderInfos = nullptr;

			if (previousPageInfos != nullptr) {
				if (previousPageInfos->subitemsRenderInfos.size() > i) {
					previousItemRenderInfos = previousPageInfos->subitemsRenderInfos[i];
				}
			}

			if (previousItemRenderInfos != nullptr) {
				if (previousItemRenderInfos->renderStatus == Success) {

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
			currentPageInfos = new itemRenderInfos();
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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutList(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	int nItems = itemInfos.item->subitems().size();

	RenderContext oldContext = _renderContext;

	QSizeF renderSize(0,0);
	_renderContext = RenderContext{itemInfos.item->direction(),
			oldContext.origin + itemInfos.item->origin(),
			oldContext.region.boundedTo(itemInfos.item->initialSize())};

	QString message = "";

	int startsId = 0;

	if (previousRender != nullptr) {
		if (previousRender->continuationIndex.canConvert<int>()) {
			startsId = previousRender->continuationIndex.toInt();

			if (previousRender->subitemsRenderInfos.isEmpty()) {
				return RenderingStatus{OtherError,
							QObject::tr("List with empty non null previous render should not occur").arg(itemInfos.item->objectName())};
			}

			if (previousRender->subitemsRenderInfos.last()->renderStatus != NotAllItemsRendered) {
				startsId++;
			} else if (previousRender->subitemsRenderInfos.last()->renderStatus == NotAllItemsRendered) {
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
		itemInfos.renderStatus = Success;
		return RenderingStatus{Success, "", renderSize};
	}

	for (int i = startsId; i < nItems; i++) {

		itemRenderInfos* subItemInfos = new itemRenderInfos();
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

		itemRenderInfos* previousInfos = nullptr;

		if (previousRender != nullptr) {
			if (i == startsId and previousRender->subitemsRenderInfos.last()->item == subItemInfos->item) {
				previousInfos = previousRender->subitemsRenderInfos.last();
			}
		}

		RenderingStatus layoutStatus = layoutItem(*subItemInfos, previousInfos);

		itemInfos.continuationIndex = i;

		if (layoutStatus.status == MissingSpace) {
			if (i == startsId) {
				itemInfos.renderStatus = MissingSpace;
				message = layoutStatus.message +
						QString("\n Table: %1 missing space to render at least one item").arg(itemInfos.item->objectName());
			} else {
				itemInfos.renderStatus = NotAllItemsRendered;
			}
			break;
		} else if (layoutStatus.status == NotAllItemsRendered) {
			itemInfos.renderStatus = NotAllItemsRendered;
			break;
		} else if (layoutStatus.status != Success) {
			itemInfos.renderStatus = layoutStatus.status;
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

	_renderContext = oldContext;

	return RenderingStatus{itemInfos.renderStatus, message, renderSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutFrame(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

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

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = oldContext.origin.x() - itemInfos.item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = oldContext.origin.y() - itemInfos.item->origin().y();
	}

	QSizeF renderSize(itemInfos.item->initialSize());
	_renderContext = RenderContext{itemInfos.item->direction(),
			origin,
			itemInfos.item->initialSize()};

	RenderingStatus status{Success, ""};

	for (int i = 0; i < nItems; i++) {

		itemRenderInfos* subItemInfos = new itemRenderInfos();
		subItemInfos->item = itemInfos.item->subitems()[i];
		subItemInfos->itemValue = itemInfos.itemValue.getValue(subItemInfos->item->dataKey());
		subItemInfos->currentSize = subItemInfos->item->initialSize();
		subItemInfos->maxSize = subItemInfos->item->maxSize();
		subItemInfos->rendered = false;
		subItemInfos->continuationIndex = QVariant();
		subItemInfos->layoutStatus = Success;

		itemInfos.subitemsRenderInfos.push_back(subItemInfos);

		itemRenderInfos* previousItemRenderInfos = nullptr;

		if (previousRender != nullptr) {
			if (previousRender->subitemsRenderInfos.size() > i) {
				previousItemRenderInfos = previousRender->subitemsRenderInfos[i];
			}
		}

		if (previousItemRenderInfos != nullptr) {
			if (previousItemRenderInfos->renderStatus == Success) {

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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutText(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (_painter == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid painter used for layout!")};
	}

	QSizeF itemInitialSize = itemInfos.item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
			itemInitialSize.height() > _renderContext.region.height()) {

		itemInfos.renderStatus = MissingSpace;
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
		status.status = MissingSpace;
		status.message = QObject::tr("Text from text block %1 overflow").arg(itemInfos.item->objectName());
	}

	itemInfos.renderStatus = status.status;
	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::layoutImage(itemRenderInfos& itemInfos, itemRenderInfos* previousRender){

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


DocumentRenderer::RenderingStatus DocumentRenderer::renderItem(itemRenderInfos& itemInfos) {

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
		return RenderingStatus{OtherError, QObject::tr("Plugins not yet supported, block : %1").arg(itemInfos.item->objectName())};
	case DocumentItem::Type::Invalid:
		return RenderingStatus{OtherError, QObject::tr("Invalid block : %1").arg(itemInfos.item->objectName())};
	}

	return RenderingStatus{OtherError, QObject::tr("Unknown error for block : %1").arg(itemInfos.item->objectName())};
}


DocumentRenderer::RenderingStatus DocumentRenderer::renderCondition(itemRenderInfos& itemInfos) {

	for (itemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
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
DocumentRenderer::RenderingStatus DocumentRenderer::renderLoop(itemRenderInfos& itemInfos) {

	RenderingStatus status{Success, ""};

	for (itemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
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
DocumentRenderer::RenderingStatus DocumentRenderer::renderPage(itemRenderInfos& itemInfos) {

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

	for (itemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
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
DocumentRenderer::RenderingStatus DocumentRenderer::renderList(itemRenderInfos& itemInfos) {

	RenderingStatus status{Success, ""};

	for (itemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
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
DocumentRenderer::RenderingStatus DocumentRenderer::renderFrame(itemRenderInfos& itemInfos) {

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

	for (itemRenderInfos* subitemInfos : itemInfos.subitemsRenderInfos) {
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
DocumentRenderer::RenderingStatus DocumentRenderer::renderText(itemRenderInfos& itemInfos) {

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
DocumentRenderer::RenderingStatus DocumentRenderer::renderImage(itemRenderInfos& itemInfos) {

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
