#ifndef NODE_STATE_H
#define NODE_STATE_H

/// COMPNENT
#include <csapex/model/memento.h>
#include <csapex/data/point.h>
#include <csapex/model/model_fwd.h>
#include <csapex/model/execution_mode.h>
#include <csapex/model/execution_type.h>
#include <csapex/utility/slim_signal.h>
#include <csapex/serialization/serializable.h>

/// SYSTEM
#include <boost/any.hpp>

namespace csapex
{

class CSAPEX_CORE_EXPORT NodeState : public Memento, public Serializable
{
    friend class SerializationBuffer;

public:
    typedef std::shared_ptr<NodeState> Ptr;
    typedef slim_signal::Signal<void()> Signal;//Impl;
//    typedef std::shared_ptr< SignalImpl > Signal;

    NodeState(const NodeHandle *parent);
    ~NodeState();

    NodeState& operator = (const NodeState& rhs);

    virtual void writeYaml(YAML::Node& out) const override;
    virtual void readYaml(const YAML::Node& node) override;

    virtual void serialize(SerializationBuffer &data) const override;
    virtual void deserialize(const SerializationBuffer& data) override;

public:
    double getMaximumFrequency() const;
    void setMaximumFrequency(double f);
    Signal max_frequency_changed;

    Point getPos() const;
    void setPos(const Point &value, bool quiet = false);
    Signal pos_changed;

    long getZ() const;
    void setZ(long value);
    Signal z_changed;

    void getColor(int& r, int& g, int &b) const;
    void setColor(int r, int g, int b);
    Signal color_changed;

    std::string getLabel() const;
    void setLabel(const std::string &label);
    Signal label_changed;

    bool isMinimized() const;
    void setMinimized(bool value);
    Signal minimized_changed;

    bool isMuted() const;
    void setMuted(bool value);
    Signal muted_changed;

    bool isEnabled() const;
    void setEnabled(bool value);
    Signal enabled_changed;

    bool isActive() const;
    void setActive(bool value);
    Signal active_changed;

    bool isFlipped() const;
    void setFlipped(bool value);
    Signal flipped_changed;

    int getThreadId() const;
    std::string getThreadName() const;
    void setThread(const std::string& name, int id);
    Signal thread_changed;

    ExecutionMode getExecutionMode() const;
    void setExecutionMode(ExecutionMode mode);
    Signal execution_mode_changed;

    ExecutionType getExecutionType() const;
    void setExecutionType(ExecutionType type);
    Signal execution_type_changed;

    int getLoggerLevel() const;
    void setLoggerLevel(int level);
    Signal logger_level_changed;


    const NodeHandle* getParent() const;
    void setParent(const NodeHandle *value);
    Signal parent_changed;

    GenericStatePtr getParameterState() const;
    void setParameterState(const GenericStatePtr &value);

    bool hasDictionaryEntry(const std::string& key) const;
    void deleteDictionaryEntry(const std::string& key);

    template <typename T>
    T getDictionaryEntry(const std::string& key) const;
    template <typename T>
    T getDictionaryEntry(const std::string& key, const T& default_value) const
    {
        if(hasDictionaryEntry(key)) {
            return getDictionaryEntry<T>(key);

        } else {
            return default_value;
        }
    }

    template <typename T>
    void setDictionaryEntry(const std::string& key, const T& value);


protected:
    virtual std::shared_ptr<Clonable> makeEmptyClone() const override;

    NodeState();

private:
    const NodeHandle* parent_;

    mutable GenericStatePtr parameter_state;

    double max_frequency_;

    std::string label_;
    Point pos_;
    long z_;

    bool minimized_;
    bool muted_;
    bool enabled_;
    bool active_;
    bool flipped_;

    int logger_level_;

    int thread_id_;
    std::string thread_name_;

    int r_;
    int g_;
    int b_;

    std::map<std::string, boost::any> dictionary;

    ExecutionMode exec_mode_;
    ExecutionType exec_type_;
};

}

#endif // NODE_STATE_H
