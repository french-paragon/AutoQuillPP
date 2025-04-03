#include "renderplugin.h"

#include <QSet>

RenderPlugin::RenderPlugin()
{

}

RenderPlugin::~RenderPlugin()
{

}

RenderPluginManager::RenderPluginManager() {

}
RenderPluginManager::~RenderPluginManager() {

	QSet<RenderPlugin*> managedPlugins(_map.begin(), _map.end());

	for (RenderPlugin* plugin : qAsConst(managedPlugins)) {
		delete plugin;
	}

}

RenderPlugin const* RenderPluginManager::getPlugin(QString const& key) const {
	return _map.value(key, nullptr);
}

bool RenderPluginManager::registerPlugin(QString const& key, RenderPlugin* plugin) {

	if (plugin == nullptr) {
		return false;
	}

	if (_map.contains(key)) {
		if (_map[key] == plugin) {
			return true;
		}
		return false;
	}

	_map.insert(key, plugin);
	return true;
}
