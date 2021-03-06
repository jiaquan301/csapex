/// HEADER
#include <csapex/view/node/node_adapter_factory.h>

/// COMPONENT
#include <csapex/model/node_facade.h>
#include <csapex/view/node/node_adapter.h>
#include <csapex/view/node/default_node_adapter.h>
#include <csapex/plugin/plugin_manager.hpp>
#include <csapex/view/param/param_adapter_factory.h>

/// SYSTEM
#include <qglobal.h>

using namespace csapex;

NodeAdapterFactory::NodeAdapterFactory(Settings& settings, PluginLocator* locator)
  : settings_(settings), plugin_locator_(locator), node_adapter_manager_(new PluginManager<NodeAdapterBuilder>("csapex::NodeAdapterBuilder"))
{
    ParameterAdapterFactory::instance().loadPlugins(locator);
}

NodeAdapterFactory::~NodeAdapterFactory()
{
    node_adapter_builders_.clear();

    delete node_adapter_manager_;
    node_adapter_manager_ = nullptr;
}

bool NodeAdapterFactory::hasAdapter(const std::string& type) const
{
    auto pos = node_adapter_builders_.find(type);
    if (pos != node_adapter_builders_.end()) {
        return pos->second != nullptr;
    } else {
        try {
            const PluginConstructor<NodeAdapterBuilder>* constructor = node_adapter_manager_->getConstructorNoThrow(type + "AdapterBuilder");
            if (constructor) {
                auto builder = constructor->construct();
                if (builder->getWrappedType() == type) {
                    return true;
                }
            }

        } catch (const std::exception&) {
            return false;
        }
    }

    return false;
}

NodeAdapter::Ptr NodeAdapterFactory::makeNodeAdapter(NodeFacadePtr node_facade, NodeBox* parent)
{
    std::string type = node_facade->getType();
    if (node_adapter_builders_.find(type) != node_adapter_builders_.end()) {
        auto& builder = node_adapter_builders_[type];
        if (builder) {
            return builder->build(node_facade, parent);
        }

    } else {
        try {
            std::string name = type + "AdapterBuilder";
            if (!node_facade->isProxy()) {
                std::string direct_name = type + "DirectAdapterBuilder";
                if (node_adapter_manager_->hasConstructor(direct_name)) {
                    name = direct_name;
                }
            }

            if (node_adapter_manager_->hasConstructor(name)) {
                const PluginConstructor<NodeAdapterBuilder>* constructor = node_adapter_manager_->getConstructorNoThrow(name);

                auto builder = constructor->construct();
                if (builder->getWrappedType() == type) {
                    node_adapter_builders_[type] = builder;
                    return builder->build(node_facade, parent);
                }
            }
            node_adapter_builders_[type] = nullptr;

        } catch (const std::exception& e) {
            std::cerr << "adapter " << type << " cannot be built: " << e.what() << std::endl;
        }
    }

    return std::make_shared<DefaultNodeAdapter>(node_facade, parent);
}

void NodeAdapterFactory::loadPlugins()
{
    if (node_adapter_builders_.empty()) {
        ensureLoaded();
    }
}

void NodeAdapterFactory::ensureLoaded()
{
    if (!node_adapter_manager_->pluginsLoaded()) {
        node_adapter_manager_->load(plugin_locator_);
    }
}
