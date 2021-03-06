#ifndef GRAPH_FACADE_IMPL_H
#define GRAPH_FACADE_IMPL_H

#include <csapex/model/graph_facade.h>

namespace csapex
{
class GraphFacadeImplementation : public GraphFacade
{
public:
    GraphFacadeImplementation(ThreadPool& executor, GraphImplementationPtr graph, SubgraphNodePtr graph_node, NodeFacadeImplementationPtr nh = nullptr, GraphFacadeImplementation* parent = nullptr);

    virtual AUUID getAbsoluteUUID() const override;

    virtual UUID generateUUID(const std::string& prefix) override;

    virtual NodeFacadePtr getNodeFacade() const override;

    virtual GraphFacadePtr getSubGraph(const UUID& uuid) override;
    GraphFacadeImplementationPtr getLocalSubGraph(const UUID& uuid);
    virtual GraphFacade* getParent() const override;

    virtual NodeFacadePtr findNodeFacade(const UUID& uuid) const override;
    virtual NodeFacadePtr findNodeFacadeNoThrow(const UUID& uuid) const noexcept override;
    virtual NodeFacadePtr findNodeFacadeForConnector(const UUID& uuid) const override;
    virtual NodeFacadePtr findNodeFacadeForConnectorNoThrow(const UUID& uuid) const noexcept override;
    virtual NodeFacadePtr findNodeFacadeWithLabel(const std::string& label) const override;

    virtual ConnectorPtr findConnector(const UUID& uuid) override;
    virtual ConnectorPtr findConnectorNoThrow(const UUID& uuid) noexcept override;

    virtual bool isConnected(const UUID& from, const UUID& to) const override;
    virtual ConnectionDescription getConnection(const UUID& from, const UUID& to) const override;
    virtual ConnectionDescription getConnectionWithId(int id) const override;

    virtual std::size_t countNodes() const override;

    virtual int getComponent(const UUID& node_uuid) const override;
    virtual int getDepth(const UUID& node_uuid) const override;

    GraphImplementationPtr getLocalGraph() const;

    GraphFacadeImplementation* getLocalParent() const;
    NodeFacadeImplementationPtr getLocalNodeFacade() const;

    virtual std::vector<UUID> enumerateAllNodes() const override;
    virtual std::vector<ConnectionDescription> enumerateAllConnections() const override;

    SubgraphNodePtr getSubgraphNode();
    TaskGenerator* getTaskGenerator(const UUID& uuid);
    ThreadPool* getThreadPool();

    void addNode(NodeFacadeImplementationPtr node);

    void clear();
    void stop();
    virtual void clearBlock() override;
    virtual void resetActivity() override;

    virtual bool isPaused() const override;
    virtual void pauseRequest(bool pause) override;

    ConnectionPtr connect(OutputPtr output, InputPtr input);

    ConnectionPtr connect(const UUID& output_id, const UUID& input_id);

    ConnectionPtr connect(NodeHandle* output, const std::string& output_name, NodeHandle* input, const std::string& input_name);
    ConnectionPtr connect(NodeHandlePtr output, const std::string& output_name, NodeHandlePtr input, const std::string& input_name);
    ConnectionPtr connect(NodeHandle* output, const std::string& output_name, const UUID& input_id);
    ConnectionPtr connect(NodeHandlePtr output, const std::string& output_name, const UUID& input_id);
    ConnectionPtr connect(const UUID& output_id, NodeHandle* input, const std::string& input_name);
    ConnectionPtr connect(const UUID& output_id, NodeHandlePtr input, const std::string& input_name);
    ConnectionPtr connect(const UUID& output_id, NodeHandlePtr input, int input_id);
    ConnectionPtr connect(NodeHandlePtr output, int output_id, const UUID& input_id);
    ConnectionPtr connect(NodeHandle* output, int output_id, NodeHandle* input, int input_id);
    ConnectionPtr connect(NodeHandlePtr output, int output_id, NodeHandlePtr input, int input_id);

    ConnectionPtr connect(NodeFacade* output, const std::string& output_name, NodeFacade* input, const std::string& input_name);
    ConnectionPtr connect(NodeFacadePtr output, const std::string& output_name, NodeFacadePtr input, const std::string& input_name);
    ConnectionPtr connect(NodeFacade* output, const std::string& output_name, const UUID& input_id);
    ConnectionPtr connect(NodeFacadePtr output, const std::string& output_name, const UUID& input_id);
    ConnectionPtr connect(const UUID& output_id, NodeFacade* input, const std::string& input_name);
    ConnectionPtr connect(const UUID& output_id, NodeFacadePtr input, const std::string& input_name);
    ConnectionPtr connect(const UUID& output_id, NodeFacadePtr input, int input_id);
    ConnectionPtr connect(NodeFacadePtr output, int output_id, const UUID& input_id);
    ConnectionPtr connect(NodeFacade* output, int output_id, NodeFacade* input, int input_id);
    ConnectionPtr connect(NodeFacadePtr output, int output_id, NodeFacadePtr input, int input_id);

    virtual std::string makeStatusString() const override;

protected:
    virtual void nodeAddedHandler(graph::VertexPtr node) override;
    virtual void nodeRemovedHandler(graph::VertexPtr node) override;

    void createSubgraphFacade(NodeFacadePtr nf);

private:
    UUID getOutputUUID(NodeFacade* node, const std::string& label);
    UUID getInputUUID(NodeFacade* node, const std::string& label);

    UUID getOutputUUID(NodeHandle* node, const std::string& label);
    UUID getInputUUID(NodeHandle* node, const std::string& label);

    template <class Container>
    UUID getOutputUUID(Container* node, int id);
    template <class Container>
    UUID getInputUUID(Container* node, int id);

    OutputPtr getOutput(const UUID& uuid);
    InputPtr getInput(const UUID& uuid);

    ConnectablePtr getConnectable(const UUID& uuid);

private:
    AUUID absolute_uuid_;

    GraphFacadeImplementation* parent_;
    NodeFacadeImplementationPtr graph_handle_;
    ThreadPool& executor_;
    GraphImplementationPtr graph_;
    SubgraphNodePtr graph_node_;

    std::unordered_map<UUID, TaskGeneratorPtr, UUID::Hasher> generators_;

    std::unordered_map<UUID, GraphFacadeImplementationPtr, UUID::Hasher> children_;

    std::unordered_map<UUID, NodeFacadePtr, UUID::Hasher> node_facades_;
};

}  // namespace csapex

#endif  // GRAPH_FACADE_IMPL_H
