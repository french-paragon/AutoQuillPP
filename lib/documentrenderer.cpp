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

    QFont font = _painter->font();
    font.setPointSizeF(item->fontSize());
    _painter->setFont(font);

    QRectF rectangle = QRectF(origin, renderSize);
    QRectF boundingRect;
    _painter->drawImage(rectangle, image);

    RenderingStatus status{Success, "", rectangle.size()};

    return status;

}
