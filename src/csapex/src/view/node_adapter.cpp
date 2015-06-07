/// HEADER
#include <csapex/view/node_adapter.h>

/// COMPONENT
#include <csapex/model/node.h>
#include <csapex/model/node_worker.h>
#include <utils_param/parameter.h>

using namespace csapex;

NodeAdapter::NodeAdapter(NodeWorkerWeakPtr adaptee, WidgetController* widget_ctrl)
    : layout_(nullptr), is_gui_setup_(false), node_(adaptee), widget_ctrl_(widget_ctrl)
{
}

NodeAdapter::~NodeAdapter()
{
}


//NodeWorker* NodeAdapter::getNodeWorker()
//{
//    return node_;
//}

void NodeAdapter::doSetupUi(QBoxLayout *layout)
{
    layout_ = layout;
    if(!is_gui_setup_) {

        try {
            setupUi(layout_);
            is_gui_setup_ = true;
        } catch(const std::exception& e) {
            std::cerr << "setting up ui for node " << node_.lock()->getUUID().getFullName() << " failed: " << e.what() << std::endl;
        }
    }
}

void NodeAdapter::stop()
{
}

Memento::Ptr NodeAdapter::getState() const
{
    Memento::Ptr state;
    return state;
}

void NodeAdapter::setParameterState(Memento::Ptr)
{

}
