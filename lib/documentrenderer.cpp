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

	if (_docTemplate == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid template")};
	}

	_writer = new QPdfWriter(filename);
	_writer->setResolution(72);
	_writer->setTitle(_docTemplate->objectName());

	if (_painter != nullptr) {
		delete _painter;
		_painter = nullptr; //need to mark that before the first page is created.
	}

	RenderingStatus status{Success, ""};

	for (DocumentItem* item : _docTemplate->subitems()) {

		DocumentValue val = dataInterface->getValue(item->dataKey());

		RenderingStatus itemStatus = renderItem(item, val);

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

DocumentRenderer::RenderingStatus DocumentRenderer::layoutDocument(QVector<itemRenderInfos> & topLevel, DocumentDataInterface* dataInterface) {

	if (_docTemplate == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid template")};
	}

	RenderingStatus status{Success, ""};

	for (DocumentItem* item : _docTemplate->subitems()) {

		DocumentValue val = dataInterface->getValue(item->dataKey());

		itemRenderInfos itemInfos;
		itemInfos.item = item;
		itemInfos.itemValue = val;
		itemInfos.currentSize = item->initialSize();
		itemInfos.maxSize = item->maxSize();
		itemInfos.rendered = false;
		itemInfos.continuationIndex = QVariant();
		itemInfos.layoutStatus = Success;
		topLevel.push_back(itemInfos);

		RenderingStatus itemStatus = layoutItem(topLevel.back());

		while (itemStatus.status == NotAllItemsRendered) {

			if (!itemStatus.message.isEmpty()) {
				if (!status.message.isEmpty()) {
					status.message += "\n";
				}
				status.message += itemStatus.message;
			}

			itemRenderInfos* previous = &topLevel.back();

			itemRenderInfos itemInfos;
			itemInfos.item = item;
			itemInfos.itemValue = val;
			itemInfos.currentSize = item->initialSize();
			itemInfos.maxSize = item->maxSize();
			itemInfos.rendered = false;
			itemInfos.continuationIndex = QVariant();
			itemInfos.layoutStatus = Success;
			topLevel.push_back(itemInfos);

			itemStatus = layoutItem(topLevel.back(), previous);

		}

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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutItem(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

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
		return layoutPage(itemInfos, previousRender);
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

	return layoutItem(*subItemInfos, subPreviousRender);
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

	if (previousRender != nullptr) {
		if (previousRender->continuationIndex.canConvert<int>()) {
			startsId = previousRender->continuationIndex.toInt();

			if (previousRender->subitemsRenderInfos.isEmpty()) {
				return RenderingStatus{OtherError,
							QObject::tr("Loop with empty non null previous render should not occur").arg(itemInfos.item->objectName())};
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
DocumentRenderer::RenderingStatus DocumentRenderer::layoutPage(itemRenderInfos& itemInfos, itemRenderInfos* previousRender) {

	if (itemInfos.item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	_renderContext = RenderContext{itemInfos.item->direction(), itemInfos.item->origin(), itemInfos.item->initialSize()};

	RenderingStatus status{Success, ""};

	int nItems = itemInfos.item->subitems().size();

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

	QSizeF renderSize(itemInfos.item->initialSize());

	QFont font = _painter->font();
	font.setPointSizeF(itemInfos.item->fontSize());
	_painter->setFont(font);

	QRectF rectangle = QRectF(origin, renderSize);
	QRectF boundingRect = _painter->boundingRect(rectangle, text);

	RenderingStatus status{Success, "", boundingRect.size()};

	if (boundingRect.width() > rectangle.width() or boundingRect.height() > rectangle.height()) {
		status.status = MissingSpace;
		status.message = QObject::tr("Text from text block %1 overflow").arg(itemInfos.item->objectName());
	}

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

	QSizeF renderSize(itemInfos.item->initialSize());

	RenderingStatus status{Success, "", renderSize};

	return status;
}


DocumentRenderer::RenderingStatus DocumentRenderer::renderItem(DocumentItem* item, DocumentValue const& val) {

	switch(item->getType()) {
	case DocumentItem::Type::Condition:
		return renderCondition(item, val);
	case DocumentItem::Type::Loop:
		return renderLoop(item, val);
	case DocumentItem::Type::Page:
		return renderPage(item, val);
	case DocumentItem::Type::List:
		return renderList(item, val);
	case DocumentItem::Type::Frame:
		return renderFrame(item, val);
	case DocumentItem::Type::Text:
		return renderText(item, val);
	case DocumentItem::Type::Image:
		return renderImage(item, val);
	case DocumentItem::Type::Plugin:
		return RenderingStatus{OtherError, QObject::tr("Plugins not yet supported, block : %1").arg(item->objectName())};
	case DocumentItem::Type::Invalid:
		return RenderingStatus{OtherError, QObject::tr("Invalid block : %1").arg(item->objectName())};
	}

	return RenderingStatus{OtherError, QObject::tr("Unknown error for block : %1").arg(item->objectName())};
}


DocumentRenderer::RenderingStatus DocumentRenderer::renderCondition(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (!val.hasMap()) {
		return RenderingStatus{MissingData, QObject::tr("Cannot read context map for condition : %1").arg(item->objectName())};
	}

	if (item->subitems().size() != 2) {
		return RenderingStatus{MissingModel, QObject::tr("Condition : %1, does not have two subitems, conditions should have exactly two subitems").arg(item->objectName())};
	}

	QString conditionKey = item->data();

	DocumentValue docdata = val.getValue(conditionKey);

	QVariant conditionData;

	if (docdata.hasData()) {
		conditionData = docdata.getValue();
	}

	bool condition = conditionData.toBool();

	DocumentItem* target_item = item->subitems()[1];

	if (condition) {
		target_item = item->subitems()[0];
	}


	DocumentValue target_val = val.getValue(target_item->dataKey());

	return renderItem(target_item, target_val);

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderLoop(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (!val.hasArray()) {
		return RenderingStatus{MissingData, QObject::tr("Cannot read context array for loop : %1").arg(item->objectName())};
	}

	if (item->subitems().size() != 1) {
		return RenderingStatus{MissingModel, QObject::tr("Loop : %1, does not have one subitem, loops should have exactly one subitem (the delegate)").arg(item->objectName())};
	}

	int nCopies = val.arraySize();

	RenderContext oldContext = _renderContext;

	QSizeF renderSize(0,0);
	_renderContext = RenderContext{item->direction(),
			oldContext.origin + item->origin(),
			oldContext.region.boundedTo(item->initialSize())};

	Status status = Success;
	QString message = "";

	for (int i = 0; i < nCopies; i++) {

		DocumentValue idxVal = val.getValue(i);
		RenderingStatus renderStatus = renderItem(item->subitems()[0], idxVal);

		if (renderStatus.status == MissingSpace) {
			if (i == 0) {
				status = MissingSpace;
				message = renderStatus.message + QString("\n Table: %1 missing space to render at least one item").arg(item->objectName());
			} else {
				status = NotAllItemsRendered;
			}
			break;
		}

		switch(_renderContext.direction) {
		case DocumentItem::Left2Right:
			_renderContext.origin.rx() += renderStatus.renderSize.width();
			_renderContext.region.rwidth() -= renderStatus.renderSize.width();
			renderSize.rwidth() += renderStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), renderStatus.renderSize.height());
            break;
		case DocumentItem::Right2Left:
			_renderContext.origin.rx() -= renderStatus.renderSize.width();
			_renderContext.region.rwidth() -= renderStatus.renderSize.width();
			renderSize.rwidth() += renderStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), renderStatus.renderSize.height());
            break;
		case DocumentItem::Top2Bottom:
			_renderContext.origin.ry() += renderStatus.renderSize.height();
            _renderContext.region.rheight() -= renderStatus.renderSize.height();
            renderSize.rwidth() = std::max(renderSize.width(), renderStatus.renderSize.width());
			renderSize.rheight() += renderStatus.renderSize.height();
            break;
		case DocumentItem::Bottom2Top:
			renderSize.rwidth() += std::max(renderSize.height(), renderStatus.renderSize.height());
			renderSize.rheight() += renderStatus.renderSize.height();
            break;
		}
	}

	_renderContext = oldContext;

	return RenderingStatus{status, message, renderSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderPage(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QPageSize pageSize(item->initialSize(), QPageSize::Unit::Point);
	QPageLayout layout;
	layout.setPageSize(pageSize);

	//ensure painting start on a new page straight after setting the page layout.
	if (_painter == nullptr) {
		_painter = new QPainter(_writer);
	} else {
		_writer->newPage();
	}

	_renderContext = RenderContext{item->direction(), item->origin(), item->initialSize()};

	RenderingStatus status{Success, ""};

    for (DocumentItem* sitem : item->subitems()) {

        DocumentValue item_val = val.getValue(sitem->dataKey());

        RenderingStatus itemStatus = renderItem(sitem, item_val);

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
DocumentRenderer::RenderingStatus DocumentRenderer::renderList(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	RenderContext oldContext = _renderContext;

	QSizeF renderSize(0,0);
	_renderContext = RenderContext{item->direction(),
			oldContext.origin + item->origin(),
			oldContext.region.boundedTo(item->initialSize())};

	Status status = Success;
	QString message = "";

	int rendered = 0;

    for (DocumentItem* sitem : item->subitems()) {

        DocumentValue item_val = val.getValue(sitem->dataKey());
        RenderingStatus renderStatus = renderItem(sitem, item_val);

		if (renderStatus.status == MissingSpace) {
			if (rendered == 0) {
				status = MissingSpace;
				message = renderStatus.message + QString("\n List: %1 missing space to render at least one item").arg(item->objectName());
			} else {
				status = NotAllItemsRendered;
			}
			break;
		}

		rendered++;

		switch(_renderContext.direction) {
		case DocumentItem::Left2Right:
			_renderContext.origin.rx() += renderStatus.renderSize.width();
			_renderContext.region.rwidth() -= renderStatus.renderSize.width();
			renderSize.rwidth() += renderStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), renderStatus.renderSize.height());
            break;
		case DocumentItem::Right2Left:
			_renderContext.origin.rx() -= renderStatus.renderSize.width();
			_renderContext.region.rwidth() -= renderStatus.renderSize.width();
			renderSize.rwidth() += renderStatus.renderSize.width();
			renderSize.rheight() = std::max(renderSize.height(), renderStatus.renderSize.height());
            break;
		case DocumentItem::Top2Bottom:
			_renderContext.origin.ry() += renderStatus.renderSize.height();
            _renderContext.region.rheight() -= renderStatus.renderSize.height();
            renderSize.rwidth() = std::max(renderSize.width(), renderStatus.renderSize.width());
			renderSize.rheight() += renderStatus.renderSize.height();
            break;
		case DocumentItem::Bottom2Top:
			renderSize.rwidth() += std::max(renderSize.height(), renderStatus.renderSize.height());
			renderSize.rheight() += renderStatus.renderSize.height();
            break;
		}
	}

	_renderContext = oldContext;

	return RenderingStatus{status, message, renderSize};

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderFrame(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	QSizeF itemInitialSize = item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
			itemInitialSize.height() > _renderContext.region.height()) {
		return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Frame: %1").arg(item->objectName())};
	}

	RenderContext oldContext = _renderContext;

	QPointF origin = oldContext.origin + item->origin();

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = oldContext.origin.x() - item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = oldContext.origin.y() - item->origin().y();
	}

	QSizeF renderSize(item->initialSize());
	_renderContext = RenderContext{item->direction(),
			origin,
			item->initialSize()};

	RenderingStatus status{Success, ""};

    for (DocumentItem* sitem : item->subitems()) {

        DocumentValue item_val = val.getValue(sitem->dataKey());

        RenderingStatus itemStatus = renderItem(sitem, item_val);

		if (itemStatus.status != Success) {
			status.status = itemStatus.status;
			if (!status.message.isEmpty()) {
				status.message += "\n";
			}
			status.message += itemStatus.message;
		} else {
			renderSize.rwidth() = std::max(status.renderSize.width(), renderSize.width());
			renderSize.rheight() = std::max(status.renderSize.height(), renderSize.height());
		}
	}

	status.renderSize = renderSize;

    _renderContext = oldContext;

	return status;


}
DocumentRenderer::RenderingStatus DocumentRenderer::renderText(DocumentItem* item, DocumentValue const& val) {

	if (item == nullptr) {
		return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
	}

	if (_painter == nullptr) {
		return RenderingStatus{OtherError, QObject::tr("Invalid painter used for rendering!")};
	}

	QSizeF itemInitialSize = item->initialSize();

	if (itemInitialSize.width() > _renderContext.region.width() or
			itemInitialSize.height() > _renderContext.region.height()) {
        return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Text: %1").arg(item->objectName())};
	}

	QVariant variant = val.getValue();
	QString text;

	if (variant.isValid()) {
		text = variant.toString();
	} else {
		text = item->data();
	}

	QPointF origin = _renderContext.origin + item->origin();

	if (_renderContext.direction == DocumentItem::Right2Left) {
		origin.rx() = _renderContext.origin.x() - item->origin().x();
	}

	if (_renderContext.direction == DocumentItem::Bottom2Top) {
		origin.ry() = _renderContext.origin.y() - item->origin().y();
	}

	QSizeF renderSize(item->initialSize());

	QFont font = _painter->font();
	font.setPointSizeF(item->fontSize());
	_painter->setFont(font);

	QRectF rectangle = QRectF(origin, renderSize);
	QRectF boundingRect;
	_painter->drawText(rectangle, 0, text, &boundingRect);

	RenderingStatus status{Success, "", boundingRect.size()};

	if (boundingRect.width() > rectangle.width() or boundingRect.height() > rectangle.height()) {
		status.status = MissingSpace;
		status.message = QObject::tr("Text from text block %1 overflow").arg(item->objectName());
	}

	return status;

}
DocumentRenderer::RenderingStatus DocumentRenderer::renderImage(DocumentItem* item, DocumentValue const& val) {

    if (item == nullptr) {
        return RenderingStatus{MissingModel, QObject::tr("Invalid item requested!")};
    }

    if (_painter == nullptr) {
        return RenderingStatus{OtherError, QObject::tr("Invalid painter used for rendering!")};
    }

    QSizeF itemInitialSize = item->initialSize();

    if (itemInitialSize.width() > _renderContext.region.width() or
        itemInitialSize.height() > _renderContext.region.height()) {
        return RenderingStatus{MissingSpace, QObject::tr("Not enough space to render Image: %1").arg(item->objectName())};
    }

    QVariant variant = val.getValue();
    QImage image;

    if (variant.isValid()) {
        if (variant.canConvert<QImage>()) {
            image = qvariant_cast<QImage>(variant);
        } else if (variant.canConvert<QString>()) {
            image = QImage(variant.toString());
        }
    } else {
        image = QImage(item->data());
    }

    if (image.isNull()) {
        return RenderingStatus{MissingData, QObject::tr("Not enough space to render Image: %1").arg(item->objectName())};
    }

    QPointF origin = _renderContext.origin + item->origin();

    if (_renderContext.direction == DocumentItem::Right2Left) {
        origin.rx() = _renderContext.origin.x() - item->origin().x();
    }

    if (_renderContext.direction == DocumentItem::Bottom2Top) {
        origin.ry() = _renderContext.origin.y() - item->origin().y();
    }

	QSizeF renderSize(item->initialSize());

	QRectF rectangle = QRectF(origin, renderSize);
    _painter->drawImage(rectangle, image);

    RenderingStatus status{Success, "", rectangle.size()};

    return status;

}
