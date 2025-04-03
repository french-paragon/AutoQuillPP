#ifndef RENDERPLUGIN_H
#define RENDERPLUGIN_H

#include "./documentrenderer.h"
#include "./documentdatainterface.h"

#include <QMap>

namespace AutoQuill {

class RenderPlugin
{
public:
	RenderPlugin();
	virtual ~RenderPlugin();

	virtual QRectF getMinimalSpace(QRectF const& availableSpace, DocumentValue const& val) const = 0;

	virtual DocumentRenderer::RenderingStatus renderItem(QRectF const& area, QPainter const& painter, DocumentValue const& val) const = 0;
};

/*!
 * \brief The RenderPluginManager class manage a collection of plugins
 */
class RenderPluginManager {

public:
	RenderPluginManager();
	~RenderPluginManager();

	RenderPlugin const* getPlugin(QString const& key) const;

	bool registerPlugin(const QString &key, RenderPlugin* plugin);

protected:

	QMap<QString, RenderPlugin*> _map;
};

} // namespace AutoQuill

#endif // RENDERPLUGIN_H
