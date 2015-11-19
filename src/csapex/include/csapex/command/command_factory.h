#ifndef COMMAND_FACTORY_H
#define COMMAND_FACTORY_H

/// PROJECT
#include <csapex/model/model_fwd.h>
#include <csapex/msg/msg_fwd.h>
#include <csapex/signal/signal_fwd.h>
#include <csapex/command/command_fwd.h>
#include <csapex/utility/uuid.h>

namespace csapex
{

class CommandFactory
{
public:
    CommandFactory(Graph* graph);

public:
    CommandPtr addConnection(const UUID& from, const UUID& to);

    CommandPtr removeAllConnectionsCmd(ConnectablePtr input);
    CommandPtr removeAllConnectionsCmd(Connectable* input);

    CommandPtr removeAllConnectionsCmd(Input* input);
    CommandPtr removeAllConnectionsCmd(Output* output);
    CommandPtr removeAllConnectionsCmd(Slot* slot);
    CommandPtr removeAllConnectionsCmd(Trigger* trigger);

    CommandPtr removeConnectionCmd(Output* output, Connection* connection);
    CommandPtr removeConnectionCmd(Trigger* trigger, Slot* other_side);



    CommandPtr deleteConnectionFulcrumCommand(int connection, int fulcrum);
    CommandPtr deleteAllConnectionFulcrumsCommand(int connection);
    CommandPtr deleteAllConnectionFulcrumsCommand(ConnectionPtr connection);
    CommandPtr deleteConnectionByIdCommand(int id);
    CommandPtr clearCommand();

private:
    Graph* graph_;
};

}

#endif // COMMAND_FACTORY_H
