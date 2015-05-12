#ifndef GRAPHIO_H
#define GRAPHIO_H

/// COMPONENT
#include <csapex/model/graph.h>
#include <csapex/csapex_fwd.h>

/// SYSTEM
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>
#include <QList>
#include <QPoint>
#include <QSize>

/// FORWARD DECLARATION

namespace csapex
{

class GraphIO : public QObject
{
    Q_OBJECT

public:
    GraphIO(GraphWorker* graph, NodeFactory* node_factory);

public:
    void saveSettings(YAML::Node& yaml);
    void loadSettings(const YAML::Node& doc);

    void saveNodes(YAML::Node &yaml);
    void loadNode(const YAML::Node &doc);

    void saveConnections(YAML::Node &yaml);
    void loadConnections(const YAML::Node& doc);

protected:
    void serializeNode(YAML::Node& doc, NodeWorker *node_worker);
    void deserializeNode(const YAML::Node& doc, NodeWorker* node_worker);

private:
    GraphWorker* graph_worker_;
    NodeFactory* node_factory_;
};

}

#endif // GRAPHIO_H
