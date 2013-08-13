#ifndef GRAPHIO_H
#define GRAPHIO_H

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
class Graph;
class Box;

class GraphIO
{
public:
    GraphIO(Graph& graph);

public:
    static const std::string extension;
    static const std::string default_config;
    static const std::string config_selector;

    static std::string defaultConfigFile();
    static std::string defaultConfigPath();

public:
    void saveSettings(YAML::Emitter &yaml);
    void loadSettings(YAML::Node& doc);

    void saveBoxes(YAML::Emitter &yaml);
    void loadBoxes(YAML::Parser &doc);

    void saveConnections(YAML::Emitter& yaml);
    void loadConnections(YAML::Node& doc);

private:
    Graph& graph_;
};

}

#endif // GRAPHIO_H