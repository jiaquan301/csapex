/// HEADER
#include <csapex/model/graph.h>

/// PROJECT
#include "ui_designer.h"
#include <csapex/model/connector.h>
#include <csapex/model/connector_in.h>
#include <csapex/model/connector_out.h>
#include <csapex/command/meta.h>
#include <csapex/command/add_box.h>
#include <csapex/command/move_box.h>
#include <csapex/command/delete_box.h>
#include <csapex/command/add_connection.h>
#include <csapex/command/delete_connection.h>
#include <csapex/command/add_connector.h>
#include <csapex/command/delete_connector.h>
#include <csapex/command/add_fulcrum.h>
#include <csapex/command/delete_fulcrum.h>
#include <csapex/command/move_fulcrum.h>
#include <csapex/model/boxed_object_constructor.h>
#include <csapex/command/dispatcher.h>
#include <csapex/manager/box_manager.h>
#include <csapex/model/box.h>
#include <csapex/model/box_group.h>
#include <csapex/utility/qt_helper.hpp>
#include <csapex/command/meta.h>
#include <csapex/command/delete_box.h>
#include <csapex/utility/stream_interceptor.h>
#include <csapex/model/template.h>
#include <csapex/manager/template_manager.h>

/// SYSTEM
#include <boost/foreach.hpp>
#include <QResizeEvent>
#include <QMenu>
#include <QScrollBar>
#include <QFileDialog>
#include <boost/algorithm/string.hpp>


using namespace csapex;

const std::string Graph::namespace_separator = ":/:";

namespace {
void split_first(const std::string& haystack, const std::string& needle,
                 /* OUTPUTS: */ std::string& lhs, std::string& rhs)
{
    size_t pos = haystack.find(needle);
    if(pos == haystack.npos) {
        return;
    }

    lhs = haystack.substr(0, pos);
    rhs = haystack.substr(pos + needle.length());
}

//void tokenize(const std::string& str, std::deque<std::string>& tokens,
//              const std::string& delimiter)
//{
//    std::string::size_type first = 0;
//    std::string::size_type last = 0;
//    while(first < str.length()) {
//        last = str.find(delimiter, first);
//        if(last == str.npos) {
//            last = str.length();
//        }
//        tokens.push_back(str.substr(first, last - first));

//        first = last + delimiter.length();
//    }


//    std::string combined;
//    foreach(const std::string& part, tokens) {
//        combined += part + delimiter;
//    }

//    combined = combined.substr(0, combined.size() - delimiter.length());

//    assert(combined == str);
//}

}

Graph::Graph()
    : dispatcher_(NULL)
{
    timer_ = new QTimer();
    timer_->setInterval(100);
    timer_->start();

    QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(tick()));
}

Graph::~Graph()
{

}

void Graph::init(CommandDispatcher *dispatcher)
{
    dispatcher_ = dispatcher;
    assert(dispatcher_);
}

std::string Graph::makeUUID(const std::string& name)
{
    int& last_id = uuids[name];
    ++last_id;

    std::stringstream ss;
    ss << name << "_" << last_id;

    return ss.str();
}


void Graph::addBox(Box::Ptr box)
{
    assert(!box->UUID().empty());
    assert(!box->getType().empty());
    assert(dispatcher_);

    boxes_.push_back(box);
    box->setCommandDispatcher(dispatcher_);

    Q_EMIT boxAdded(box.get());
}

void Graph::deleteBox(const std::string& uuid)
{
    Box::Ptr box = findBox(uuid);

    box->stop();

    std::vector<Box::Ptr>::iterator it = std::find(boxes_.begin(), boxes_.end(), box);
    if(it != boxes_.end()) {
        boxes_.erase(it);
    }

    Q_EMIT boxDeleted(box.get());

    box->deleteLater();
}

Template::Ptr Graph::toTemplate(const std::string& name) const
{
    Template::Ptr sub_graph_templ = TemplateManager::instance().createNewNamedTemplate(name);
    std::vector<std::pair<std::string, std::string> > connections;
    generateTemplate(sub_graph_templ, connections, false);

    return sub_graph_templ;
}

Template::Ptr Graph::convertSelectionToTemplate(std::vector<std::pair<std::string, std::string> >& connections) const
{
    Template::Ptr sub_graph_templ = TemplateManager::instance().createNewTemporaryTemplate();
    generateTemplate(sub_graph_templ, connections, true);

    return sub_graph_templ;
}


Template::Ptr Graph::generateTemplate(Template::Ptr templ, std::vector<std::pair<std::string, std::string> >& connections, bool only_selected) const
{
    std::vector<Box*> selected;

    std::map<std::string, std::string> old_box_to_new_box;

    foreach(Box::Ptr b, boxes_) {
        // iterate selected boxes
        if(b->isSelected() || !only_selected) {
//            if(only_selected) {
                selected.push_back(b.get());
//            }

            Box::State::Ptr state = boost::dynamic_pointer_cast<Box::State>(b->getState());
            std::string new_uuid = templ->addBox(b->getType(), b->pos(), state);

            size_t start_pos = new_uuid.find(Template::PARENT_PREFIX_PATTERN);
            assert(start_pos != std::string::npos);

            old_box_to_new_box[b->UUID()] = new_uuid;
        }
    }

    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected() || !only_selected) {
            foreach(ConnectorIn* in, b->input) {
                if(in->isConnected()) {
                    Connector* target = in->getConnected();
                    Box* owner = target->getBox();

                    bool owner_is_selected = false;
                        foreach(Box* b, selected) {
                            owner_is_selected |= (b == owner);
                        }

                    bool is_external = !owner_is_selected;
                    // internal connections are done by the next loop
                    // external connections should be split
                    if(is_external) {
                        std::cerr << "  > split incoming connection between " << in->UUID() << " and " << target->UUID() << std::endl;

                        std::string new_connector_uuid = templ->addConnector(in->getLabel(), in->getType()->name(), true, true);

                        std::string in_box, in_connector;
                        split_first(in->UUID(), Connector::namespace_separator, in_box, in_connector);
                        templ->addConnection(new_connector_uuid, old_box_to_new_box[b->UUID()] + Connector::namespace_separator + in_connector);

                        connections.push_back(std::make_pair(target->UUID(), new_connector_uuid));
                    }
                }
            }
            foreach(ConnectorOut* out, b->output) {
                std::string new_connector_uuid;

                for(ConnectorOut::TargetIterator it = out->beginTargets(); it != out->endTargets(); ++it) {
                    ConnectorIn* in = *it;
                    Box* owner = in->getBox();

                    bool is_selected = false;
                        foreach(Box* b, selected) {
                            is_selected |= (b == owner);
                        }

                    bool is_external = !is_selected;
                    if(is_external) {
                        // external connections are split
                        std::cerr << "  > split outgoing connection between " << in->UUID() << " and " << out->UUID() << std::endl;

                        if(new_connector_uuid.empty()) {
                            new_connector_uuid = templ->addConnector(out->getLabel(), out->getType()->name(), false, true);
                        }

                        std::string out_box, out_connector;
                        split_first(out->UUID(), Connector::namespace_separator, out_box, out_connector);
                        templ->addConnection(old_box_to_new_box[b->UUID()] + Connector::namespace_separator + out_connector, new_connector_uuid);

                        connections.push_back(std::make_pair(new_connector_uuid, in->UUID()));

                    } else {
                        // internal connections are kept
                        std::cerr << "  > keep internal connection between " << in->UUID() << " and " << out->UUID() << std::endl;

                        std::string in_box, in_connector;
                        split_first(in->UUID(), Connector::namespace_separator, in_box, in_connector);

                        std::string out_box, out_connector;
                        split_first(out->UUID(), Connector::namespace_separator, out_box, out_connector);

                        std::string in = old_box_to_new_box[in_box] + Connector::namespace_separator + in_connector;
                        std::string out = old_box_to_new_box[out_box] + Connector::namespace_separator + out_connector;

                        templ->addConnection(out, in);
                    }
                }
            }

        }
    }

    return templ;
}

Command::Ptr Graph::moveSelectedBoxes(const QPoint& delta)
{
    command::Meta::Ptr meta(new command::Meta);

    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            meta->add(Command::Ptr(new command::MoveBox(b.get(), b->pos())));
        }
    }

    foreach(const Connection::Ptr& connection, visible_connections) {
        if(connection->from()->getBox()->isSelected() && connection->to()->getBox()->isSelected()) {
            int n = connection->getFulcrumCount();
            for(int i = 0; i < n; ++i) {
                QPoint pos = connection->getFulcrum(i);
                meta->add(Command::Ptr(new command::MoveFulcrum(connection->id(), i, pos - delta, pos)));
            }
        }
    }

    return meta;
}

bool Graph::addConnection(Connection::Ptr connection)
{
    if(connection->from()->tryConnect(connection->to())) {
        Connector* from = findConnector(connection->from()->UUID());
        Connector* to = findConnector(connection->to()->UUID());

        Graph::Ptr graph_from = from->getBox()->getCommandDispatcher()->getGraph();
        Graph::Ptr graph_to = to->getBox()->getCommandDispatcher()->getGraph();

//        if(!graph_from->isHidden() && !graph_to->isHidden()) {
        if(graph_from.get() == this && graph_to.get() == this) {
            visible_connections.push_back(connection);
        }

        Q_EMIT connectionAdded(connection.get());
        return true;
    }

    std::cerr << "cannot connect " << connection->from()->UUID() << " (" <<( connection->from()->isInput() ? "i": "o" )<< ") to " << connection->to()->UUID() << " (" <<( connection->to()->isInput() ? "i": "o" )<< ")"  << std::endl;
    return false;
}

void Graph::deleteConnection(Connection::Ptr connection)
{
    connection->from()->removeConnection(connection->to());

    for(std::vector<Connection::Ptr>::iterator c = visible_connections.begin(); c != visible_connections.end();) {
        if(*connection == **c) {
            visible_connections.erase(c);
            connection->to()->setError(false);

            Q_EMIT connectionDeleted(connection.get());
        } else {
            ++c;
        }
    }

    Q_EMIT stateChanged();
}

void Graph::stop()
{
    foreach(Box::Ptr box, boxes_) {
        box->stop();
    }

    boxes_.clear();
}


Command::Ptr Graph::clear()
{
    command::Meta::Ptr clear(new command::Meta);

    foreach(Box::Ptr box, boxes_) {
        Command::Ptr cmd(new command::DeleteBox(box->UUID()));
        clear->add(cmd);
    }

    return clear;
}

void Graph::reset()
{
    uuids.clear();
    boxes_.clear();
    connectors_.clear();
    visible_connections.clear();
}

Graph::Ptr Graph::findSubGraph(const std::string& uuid)
{
    Box::Ptr bg = findBox(uuid);
    assert(bg);
    assert(bg->hasSubGraph());

    return bg->getSubGraph();
}

Box::Ptr Graph::findBox(const std::string &box_uuid)
{
    Box::Ptr box = findBoxNoThrow(box_uuid);

    if(box) {
        return box;
    }

    std::cerr << "cannot find box \"" << box_uuid << "\n";
    std::cerr << "available boxes:\n";
    foreach(Box::Ptr b, boxes_) {
        std::cerr << b->UUID() << '\n';
    }
    std::cerr << std::endl;
    throw std::runtime_error("cannot find box");

}

Box::Ptr Graph::findBoxNoThrow(const std::string &box_uuid)
{
    foreach(Box::Ptr b, boxes_) {
        if(b->UUID() == box_uuid) {
            return b;
        }
    }

    foreach(Box::Ptr b, boxes_) {
        BoxGroup::Ptr grp = boost::dynamic_pointer_cast<BoxGroup> (b);
        if(grp) {
            Box::Ptr tmp = grp->getSubGraph()->findBoxNoThrow(box_uuid);
            if(tmp) {
                return tmp;
            }
        }
    }

    return Box::NullPtr;
}


Box::Ptr Graph::findConnectorOwner(const std::string &uuid)
{
    std::string l, r;
    split_first(uuid, Connector::namespace_separator, l, r);

    try {
        return findBox(l);

    } catch(const std::exception& e) {
        std::cerr << "error: cannot find owner of connector '" << uuid << "'\n";

        foreach(Box::Ptr b, boxes_) {
            std::cerr << "box: " << b->UUID() << "\n";
            std::cerr << "inputs: " << "\n";
            foreach(ConnectorIn* in, b->input) {
                std::cerr << "\t" << in->UUID() << "\n";
            }
            std::cerr << "outputs: " << "\n";
            foreach(ConnectorOut* out, b->output) {
                std::cerr << "\t" << out->UUID() << "\n";
            }
        }

        std::cerr << std::flush;

        throw std::runtime_error(std::string("cannot find owner of connector \"") + uuid);
    }
}

Connector* Graph::findConnector(const std::string &uuid)
{
    std::string l, r;
    split_first(uuid, Connector::namespace_separator, l, r);

    Box::Ptr owner = findBox(l);
    assert(owner);

    Connector* result = NULL;
    result = owner->getInput(uuid);

    if(result == NULL) {
        result = owner->getOutput(uuid);
    }

    assert(result);

    return result;
}

bool Graph::handleConnectionSelection(int id, bool add)
{
    if(id != -1) {
        if(add) {
            if(isConnectionWithIdSelected(id)) {
                deselectConnectionById(id);
            } else {
                selectConnectionById(id, true);
            }
        } else {
            if(isConnectionWithIdSelected(id)) {
                if(noSelectedConnections() == 1) {
                    deselectConnectionById(id);
                } else {
                    selectConnectionById(id);
                }
            } else {
                selectConnectionById(id);
            }
        }
        return false;

    } else if(!add) {
        deselectConnections();
        return false;
    }

    return true;
}

void Graph::handleBoxSelection(Box* box, bool add)
{
    if(box != NULL) {
        if(add) {
            if(box->isSelected()) {
                box->setSelected(false);
            } else {
                selectBox(box, true);
            }
        } else {
            if(box->isSelected()) {
                deselectBoxes();
                if(noSelectedBoxes() != 1) {
                    selectBox(box);
                }
            } else {
                selectBox(box);
            }
        }
    } else if(!add) {
        deselectBoxes();
    }
}


Connection::Ptr Graph::getConnectionWithId(int id)
{
    BOOST_FOREACH(Connection::Ptr& connection, visible_connections) {
        if(connection->id() == id) {
            return connection;
        }
    }

    return Connection::NullPtr;
}

Connection::Ptr Graph::getConnection(Connection::Ptr c)
{
    BOOST_FOREACH(Connection::Ptr& connection, visible_connections) {
        if(*connection == *c) {
            return connection;
        }
    }

    std::cerr << "error: cannot get connection for " << *c << std::endl;

    return Connection::NullPtr;
}

int Graph::getConnectionId(Connection::Ptr c)
{
    Connection::Ptr internal = getConnection(c);

    if(internal != Connection::NullPtr) {
        return internal->id();
    }

    std::cerr << "error: cannot get connection id for " << *c << std::endl;

    return -1;
}

int Graph::noSelectedConnections()
{
    int c = 0;
    foreach(const Connection::Ptr& connection, visible_connections) {
        if(connection->isSelected()) {
            ++c;
        }
    }

    return c;
}

void Graph::deselectConnections()
{
    BOOST_FOREACH(Connection::Ptr& connection, visible_connections) {
        connection->setSelected(false);
    }
}

Command::Ptr Graph::deleteConnectionByIdCommand(int id)
{
    foreach(const Connection::Ptr& connection, visible_connections) {
        if(connection->id() == id) {
            return Command::Ptr(new command::DeleteConnection(connection->from(), connection->to()));
        }
    }

    return Command::Ptr();
}

Command::Ptr Graph::deleteConnectionFulcrumCommand(int connection, int fulcrum)
{
    return Command::Ptr(new command::DeleteFulcrum(connection, fulcrum));
}

Command::Ptr Graph::deleteAllConnectionFulcrumsCommand(int connection)
{
    command::Meta::Ptr meta(new command::Meta);
    int n = getConnectionWithId(connection)->getFulcrumCount();
    for(int i = n - 1; i >= 0; --i) {
        meta->add(deleteConnectionFulcrumCommand(connection, i));
    }

    return meta;
}

Command::Ptr Graph::deleteAllConnectionFulcrumsCommand(Connection::Ptr connection)
{
    return deleteAllConnectionFulcrumsCommand(getConnectionId(connection));
}


Command::Ptr Graph::deleteConnectionById(int id)
{
    Command::Ptr cmd(deleteConnectionByIdCommand(id));

    return cmd;
}

Command::Ptr Graph::deleteSelectedConnections()
{
    command::Meta::Ptr meta(new command::Meta);

    foreach(const Connection::Ptr& connection, visible_connections) {
        if(isConnectionWithIdSelected(connection->id())) {
            meta->add(Command::Ptr(new command::DeleteConnection(connection->from(), connection->to())));
        }
    }

    deselectConnections();

    return meta;
}

void Graph::selectConnectionById(int id, bool add)
{
    if(!add) {
        BOOST_FOREACH(Connection::Ptr& connection, visible_connections) {
            connection->setSelected(false);
        }
    }
    Connection::Ptr c = getConnectionWithId(id);
    if(c != Connection::NullPtr) {
        c->setSelected(true);
    }
}


void Graph::deselectConnectionById(int id)
{
    BOOST_FOREACH(Connection::Ptr& connection, visible_connections) {
        if(connection->id() == id) {
            connection->setSelected(false);
        }
    }
}


bool Graph::isConnectionWithIdSelected(int id)
{
    if(id < 0) {
        return false;
    }

    foreach(const Connection::Ptr& connection, visible_connections) {
        if(connection->id() == id) {
            return connection->isSelected();
        }
    }

    std::stringstream ss; ss << "no connection with id " << id;
    throw std::runtime_error(ss.str());
}

Command::Ptr Graph::deleteSelectedBoxes()
{
    command::Meta::Ptr meta(new command::Meta);

    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            meta->add(Command::Ptr(new command::DeleteBox(b->UUID())));
        }
    }

    deselectBoxes();

    return meta;
}

Command::Ptr Graph::groupSelectedBoxes()
{
    // TODO: grouping does not work recursively, because groups cannot yet be deleted / created via commands
    QPoint tl(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            QPoint pos = b->pos();
            if(pos.x() < tl.x()) {
                tl.setX(pos.x());
            }
            if(pos.y() < tl.y()) {
                tl.setY(pos.y());
            }
        }
    }


    std::vector<std::pair<std::string, std::string> > connections;
    Template::Ptr templ = convertSelectionToTemplate(connections);

    std::string type = std::string("::template::") + templ->getName();

    std::string group_uuid = makeUUID(type);

    command::Meta::Ptr meta(new command::Meta);

    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            meta->add(Command::Ptr(new command::DeleteBox(b->UUID())));
        }
    }

    meta->add(command::AddBox::Ptr(new command::AddBox(type, tl, "", group_uuid)));

    typedef std::pair<std::string, std::string> PAIR;
    foreach(const PAIR& c, connections) {
        std::string from = Template::fillInTemplate(c.first, group_uuid);
        std::string to = Template::fillInTemplate(c.second, group_uuid);
        meta->add(Command::Ptr(new command::AddConnection(from, to)));
    }

    return meta;
}


void Graph::selectAll()
{
    foreach(Box::Ptr b, boxes_) {
        b->setSelected(true);
    }
}

void Graph::clearSelection()
{
    foreach(Box::Ptr b, boxes_) {
        b->setSelected(false);
    }
}

void Graph::toggleBoxSelection(Box *box)
{
    bool shift = Qt::ShiftModifier == QApplication::keyboardModifiers();

    handleBoxSelection(box, shift);
}


int Graph::noSelectedBoxes()
{
    int c = 0;

    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            ++c;
        }
    }

    return c;
}

void Graph::boxMoved(Box *box, int dx, int dy)
{
    if(box->isSelected() && box->hasFocus()) {
        foreach(Box::Ptr b, boxes_) {
            if(b.get() != box && b->isSelected()) {
                b->move(b->x() + dx, b->y() + dy);
            }
        }
        foreach(const Connection::Ptr& connection, visible_connections) {
            if(connection->from()->getBox()->isSelected() && connection->to()->getBox()->isSelected()) {
                int n = connection->getFulcrumCount();
                for(int i = 0; i < n; ++i) {
                    connection->moveFulcrum(i, connection->getFulcrum(i) + QPoint(dx,dy));
                }
            }
        }
    }
}


void Graph::deselectBoxes()
{
    foreach(Box::Ptr b, boxes_) {
        if(b->isSelected()) {
            b->setSelected(false);
        }
    }
}

void Graph::selectBox(Box *box, bool add)
{
    assert(!box->isSelected());

    if(!add) {
        deselectBoxes();
    }

    box->setSelected(true);
}

void Graph::tick()
{
    foreach(Box::Ptr b, boxes_) {
        b->tick();
    }
    foreach(const Connection::Ptr& connection, visible_connections) {
        connection->tick();
    }
}