/// HEADER
#include <csapex/command/add_msg_connection.h>

/// COMPONENT
#include <csapex/command/command.h>
#include <csapex/model/node_constructor.h>
#include <csapex/model/graph_facade.h>
#include <csapex/model/graph.h>
#include <csapex/utility/assert.h>
#include <csapex/msg/bundled_connection.h>
#include <csapex/msg/output.h>
#include <csapex/msg/input.h>
#include <csapex/model/node_handle.h>

using namespace csapex;
using namespace csapex::command;

AddMessageConnection::AddMessageConnection(const AUUID& parent_uuid, const UUID& from_uuid, const UUID& to_uuid)
    : AddConnection(parent_uuid, from_uuid, to_uuid), from(nullptr), to(nullptr)
{
}

bool AddMessageConnection::doExecute()
{
    if(from == nullptr) {
        refresh();
    }

    Graph* graph = getGraph();

    return graph->addConnection(BundledConnection::connect(from, to));
}

void AddMessageConnection::refresh()
{
    Graph* graph = getGraph();

    Connectable* f = graph->findConnector(from_uuid);
    Connectable* t = graph->findConnector(to_uuid);

    apex_assert_hard(!f->isVirtual());
    apex_assert_hard(!t->isVirtual());

    apex_assert_hard((f->isOutput() && t->isInput()));
    from = dynamic_cast<Output*>(f);
    to =  dynamic_cast<Input*>(t);

    apex_assert_hard(from);
    apex_assert_hard(to);
}
