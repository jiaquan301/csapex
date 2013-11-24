#ifndef NODE_H_
#define NODE_H_

/// COMPONENT
#include <csapex/csapex_fwd.h>
#include <csapex/model/tag.h>
#include <csapex/model/memento.h>
#include <csapex/model/error_state.h>
#include <csapex/model/generic_state.h>

/// PROJECT
#include <utils_param/parameter.h>

/// SYSTEM
#include <QObject>
#include <QIcon>
#include <QTreeWidgetItem>

namespace csapex {

class Node : public QObject, public ErrorState
{
    Q_OBJECT

    friend class NodeState;
    friend class Box;
    friend class GraphIO;
    friend class Graph;
    friend class NodeConstructor;

    friend class command::AddConnector;

public:
    typedef boost::shared_ptr<Node> Ptr;

public:
    Node(const std::string& uuid = "");
    virtual ~Node();
    virtual void setup();

    void setType(const std::string& type);
    std::string getType() const;

    void setCategory(const std::string& category) __attribute__ ((deprecated));

    void addTag(const Tag& tag);
    std::vector<Tag> getTags() const;

    void addParameter(const param::Parameter& param);
    void addParameter(const param::Parameter::Ptr& param);

    std::vector<param::Parameter::Ptr> getParameters() const;

    template <typename T>
    T param(const std::string& name) const
    {
        try {
            return getParameter(name)->as<T>();
        } catch(const std::out_of_range& e) {
            throw std::runtime_error(std::string("unknown parameter '") + name + "'");
        }
    }
    param::Parameter::Ptr getParameter(const std::string& name) const;

    void setIcon(QIcon icon);
    QIcon getIcon() const;

    void stop();

private:
    void setUUID(const std::string& uuid);

    void connectConnector(Connector* c);
    void disconnectConnector(Connector* c);

public:
    std::string UUID() const;

    virtual bool canBeDisabled() const;
    bool isEnabled();

private:
    void setNodeState(NodeStatePtr memento);

public:
    void setNodeStateLater(NodeStatePtr state);
    NodeStatePtr getNodeState();

    /// TODO: get rid of this
    virtual void setBox(Box* box);
    Box* getBox() const;
    NodeWorker* getNodeWorker() const;

    template <typename T>
    ConnectorIn* addInput(const std::string& label, bool optional = false) {
        return addInput(T::make(), label, optional);
    }

    template <typename T>
    ConnectorOut* addOutput(const std::string& label) {
        return addOutput(T::make(), label);
    }

    ConnectorIn* addInput(ConnectionTypePtr type, const std::string& label, bool optional);
    ConnectorOut* addOutput(ConnectionTypePtr type, const std::string& label);

    void addInput(ConnectorIn* in) __attribute__ ((deprecated));
    void addOutput(ConnectorOut* out) __attribute__ ((deprecated));

    int countInputs() const;
    int countOutputs() const;

    ConnectorIn* getInput(const unsigned int index) const;
    ConnectorOut* getOutput(const unsigned int index) const;

    ConnectorIn* getInput(const std::string& uuid) const;
    ConnectorOut* getOutput(const std::string& uuid) const;

    void removeInput(ConnectorIn *in);
    void removeOutput(ConnectorOut *out);

    void setSynchronizedInputs(bool sync);

    int nextInputId();
    int nextOutputId();

    QPoint getPosition() const;
    void setPosition(const QPoint& pos);

    CommandDispatcher* getCommandDispatcher() const;
    void setCommandDispatcher(CommandDispatcher* d);

    CommandPtr removeAllConnectionsCmd();

    QTreeWidgetItem* createDebugInformation() const;

    ///
    /// IO
    ///

    YAML::Emitter& save(YAML::Emitter& out) const;
    void read(const YAML::Node &doc);

protected:
    virtual void setState(Memento::Ptr memento);
    virtual Memento::Ptr getState() const;

    void makeThread();

private:
    void errorEvent(bool error, const std::string &msg, ErrorLevel level);

    void registerInput(ConnectorIn* in);
    void registerOutput(ConnectorOut* out);

    QTreeWidgetItem * createDebugInformationConnector(Connector *connector) const;

public Q_SLOTS:
    virtual void messageArrived(ConnectorIn* source);
    virtual void allConnectorsArrived();

    virtual void enable(bool e);
    virtual void enable();
    virtual void disable(bool e);
    virtual void disable();
    virtual void connectorChanged();

    virtual void tick();

    virtual void updateModel();
    void eventGuiChanged();

    void messageProcessed();

    void killContent();

    void enableIO(bool enable);
    void setIOError(bool error);
    void setLabel(const std::string& label);
    void setMinimized(bool min);

Q_SIGNALS:
    void stateChanged();
    void modelChanged();
    void toggled(bool);
    void started();

    void connectionInProgress(Connector*, Connector*);
    void connectionDone();
    void connectionStart();

    void connectorCreated(Connector*);
    void connectorRemoved(Connector*);

    void connectorEnabled(Connector*);
    void connectorDisabled(Connector*);

protected:
    std::string type_;
    mutable std::vector<Tag> tags_;
    QIcon icon_;

private:
    Box* box_;

    QThread* private_thread_;
    QMutex worker_mutex_;

    NodeWorker* worker_;
    std::string uuid_;

    NodeStatePtr node_state_;

    std::vector<ConnectorIn*> input;
    std::vector<ConnectorOut*> output;

    CommandDispatcher* dispatcher_;

    bool loaded_state_available_;

    GenericState state;
};

}

#endif // NODE_H_