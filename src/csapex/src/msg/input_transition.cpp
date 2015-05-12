/// HEADER
#include <csapex/msg/input_transition.h>

/// COMPONENT
#include <csapex/model/node_worker.h>
#include <csapex/model/connection.h>
#include <csapex/msg/input.h>
#include <csapex/msg/dynamic_input.h>
#include <csapex/msg/output.h>

/// SYSTEM
#include <sstream>

using namespace csapex;

InputTransition::InputTransition(NodeWorker* node)
    : Transition(node), one_input_is_dynamic_(false)
{
}

void InputTransition::establish()
{
    std::unique_lock<std::recursive_mutex> lock(sync);
    auto unestablished_connections = unestablished_connections_;
    lock.unlock();

    if(!unestablished_connections.empty()) {
        for(auto cw : unestablished_connections) {
            ConnectionPtr c = cw.lock();
            if(!c->isSinkEstablished()) {
                c->establishSink();
            }
            if(c->isSourceEstablished() && c->isSinkEstablished()) {
                establishConnection(cw);
            }
        }
    }
}

void InputTransition::connectionAdded(Connection *connection)
{

    connection->new_message.connect([this]() {
        //        std::cerr << "new message in " << node_->getUUID() << std::endl;
        //        update();
        node_->triggerCheckTransitions();
    });

    connection->endpoint_established.connect([this]() {
        // establish();
        node_->triggerCheckTransitions();
    });

    one_input_is_dynamic_ = false;
    for(Input* i : node_->getMessageInputs()) {
        if(i->isDynamic()) {
            one_input_is_dynamic_ = true;
            break;
        }
    }
}

void InputTransition::fireIfPossible()
{
    if(!node_->canProcess()) {
        return;
    }

    if(node_->isSource()) {
        apex_assert_hard(established_connections_.empty());
        //fire(); -> instead of tick!!!!
    } else {
        if(!isConnection(Connection::State::READY_TO_RECEIVE) &&
                areConnections(Connection::State::UNREAD, Connection::State::READ/*, Connection::State::DONE*/) &&
                !areConnections(Connection::State::READ)) {
            if(node_->getState() == NodeWorker::State::ENABLED) {

                for(const auto& c : established_connections_) {
                    auto connection = c.lock();
                    if(connection->getState() == Connection::State::NOT_INITIALIZED) {
                        if(connection->isEstablished()) {
                            return;
                        }
                    }
                }

                fire();
            }
        }
    }
}

void InputTransition::notifyMessageProcessed()
{
    apex_assert_hard(unestablished_connections_.empty());
    apex_assert_hard(areConnections(Connection::State::READ));

    bool has_multipart = false;
    bool multipart_are_done = true;

    for(auto& connection : established_connections_) {
        ConnectionPtr c = connection.lock();
        int f = c->getMessage()->flags.data;
        if(f & (int) ConnectionType::Flags::Fields::MULTI_PART) {
            has_multipart = true;

            bool last_part = f & (int) ConnectionType::Flags::Fields::LAST_PART;
            multipart_are_done &= last_part;
        }
    }

    if(!multipart_are_done) {
        for(auto& connection : established_connections_) {
            ConnectionPtr c = connection.lock();
            int f = c->getMessage()->flags.data;

            if(f & (int) ConnectionType::Flags::Fields::MULTI_PART) {
                c->setState(Connection::State::DONE);
                c->notifyMessageProcessed();
            }
        }

    } else {
        for(auto& connection : established_connections_) {
            ConnectionPtr c = connection.lock();
            c->setState(Connection::State::DONE);
        }
        apex_assert_hard(areConnections(Connection::State::DONE));

        for(auto& connection : established_connections_) {
            ConnectionPtr c = connection.lock();
            c->notifyMessageProcessed();
        }
    }
}

void InputTransition::fire()
{
    apex_assert_hard(node_->thread() == QThread::currentThread());
    apex_assert_hard(node_->canProcess());
    apex_assert_hard(node_->getState() == NodeWorker::State::ENABLED);
    apex_assert_hard(!isConnection(Connection::State::DONE));
    //    apex_assert_hard(!isConnection(Connection::State::NOT_INITIALIZED));
    apex_assert_hard(!isConnection(Connection::State::READY_TO_RECEIVE));
    apex_assert_hard(established_connections_.empty() || !areConnections(Connection::State::READ));

    std::vector<DynamicInput*> dynamic_inputs;
    ConnectionTypeConstPtr dynamic_message_part;
    ConnectionPtr dynamic_connection;

    for(Input* input : node_->getMessageInputs()) {
        if(input->isDynamic() && input->isConnected()) {
            auto connections = input->getConnections();
            apex_assert_hard(connections.size() == 1);
            dynamic_connection = connections.front().lock();
            auto s = dynamic_connection->getState();
            apex_assert_hard(s == Connection::State::READ ||
                             s == Connection::State::UNREAD);
            dynamic_message_part = dynamic_connection->getMessage();
            apex_assert_hard(dynamic_message_part != nullptr);
            DynamicInput* di = dynamic_cast<DynamicInput*>(input);
            dynamic_inputs.push_back(di);
        }
    }

    apex_assert_hard(dynamic_inputs.size() <= 1);
    if(!dynamic_inputs.empty()) {
        DynamicInput* di = dynamic_inputs.front();
        bool every_part_received = di->inputMessagePart(dynamic_message_part);
        if(!every_part_received) {
            // wait until everything is here, then proceed.
            auto s = dynamic_connection->getState();
            apex_assert_hard(s == Connection::State::UNREAD ||
                             s == Connection::State::READ);
            dynamic_connection->setState(Connection::State::READ);
            dynamic_connection->setState(Connection::State::DONE);
            dynamic_connection->notifyMessageProcessed();
            return;
        }
    }

    //    std::cerr << "fire " <<  node_->getUUID() << std::endl;

    for(Input* input : node_->getMessageInputs()) {
        //        std::cerr << "input message from " <<  node_->getUUID() << " -> " << input->getUUID() << std::endl;

        if(input->isConnected()) {
            if(input->isDynamic()) {
                DynamicInput* di = dynamic_cast<DynamicInput*>(input);
                di->composeMessage();

            } else {
                auto connections = input->getConnections();
                apex_assert_hard(connections.size() == 1);
                auto connection = connections.front().lock();
                auto s = connection->getState();
                apex_assert_hard(s == Connection::State::READ ||
                                 s == Connection::State::UNREAD ||
                                 s == Connection::State::DONE);
                auto msg = connection->getMessage();
                apex_assert_hard(msg != nullptr);
                input->inputMessage(msg);
            }
        } else {
            input->inputMessage(connection_types::makeEmpty<connection_types::NoMessage>());
        }
    }

    for(auto& connection : established_connections_) {
        ConnectionPtr c = connection.lock();
        auto s = c->getState();
        apex_assert_hard(c->isEstablished());
        apex_assert_hard(s != Connection::State::NOT_INITIALIZED);
        apex_assert_hard(s != Connection::State::READY_TO_RECEIVE);
        apex_assert_hard(s == Connection::State::UNREAD ||
                         s == Connection::State::READ ||
                         s == Connection::State::DONE);
    }

    apex_assert_hard(!areConnections(Connection::State::DONE));
    for(auto& connection : established_connections_) {
        ConnectionPtr c = connection.lock();
        if(c->getState() != Connection::State::DONE) {
            c->setState(Connection::State::READ);
        }
    }

    //    std::cerr << "-> process " <<  node_->getUUID() << std::endl;

    apex_assert_hard(node_->getState() == NodeWorker::State::ENABLED);
    node_->triggerProcess();
}