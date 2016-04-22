#ifndef NODE_MODIFIER_H
#define NODE_MODIFIER_H

/// COMPONENT
#include <csapex/model/model_fwd.h>
#include <csapex/msg/msg_fwd.h>
#include <csapex/signal/signal_fwd.h>
#include <csapex/model/multi_connection_type.h>
#include <csapex/msg/message.h>
#include <csapex/msg/message_traits.h>
#include <csapex/utility/uuid.h>


namespace csapex
{
class NodeModifier
{
public:
    NodeModifier();
    virtual ~NodeModifier();

    void setNodeWorker(NodeWorker* worker);

    /*
     * MESSAGES
     */

    /// "real" messages
    template <typename T>
    Input* addInput(const std::string& label,
                    typename std::enable_if<std::is_base_of<Token, T>::value >::type* = 0) {
        return addInput(connection_types::makeEmptyMessage<T>(), label, false, false);
    }
    template <typename T>
    Input* addOptionalInput(const std::string& label,
                            typename std::enable_if<std::is_base_of<Token, T>::value >::type* = 0) {
        return addInput(connection_types::makeEmptyMessage<T>(), label, false, true);
    }
    template <typename T>
    Input* addDynamicInput(const std::string& label,
                           typename std::enable_if<std::is_base_of<Token, T>::value >::type* = 0) {
        return addInput(connection_types::makeEmptyMessage<T>(), label, true, false);
    }
    template <typename T>
    Output* addOutput(const std::string& label,
                      typename std::enable_if<std::is_base_of<Token, T>::value >::type* = 0) {
        return addOutput(connection_types::makeEmptyMessage<T>(), label, false);
    }
    template <typename T>
    Output* addDynamicOutput(const std::string& label,
                             typename std::enable_if<std::is_base_of<Token, T>::value >::type* = 0) {
        return addOutput(connection_types::makeEmptyMessage<T>(), label, true);
    }

    /// "container" messages
    template <typename Container, typename T>
    Input* addInput(const std::string& label) {
        Container::template registerType<T>();
        return addInput(Container::template make<T>(), label, false, false);
    }
    template <typename Container, typename T>
    Input* addOptionalInput(const std::string& label) {
        Container::template registerType<T>();
        return addInput(Container::template make<T>(), label, false, true);
    }
    template <typename Container, typename T>
    Input* addDynamicInput(const std::string& label) {
        Container::template registerType<T>();
        return addInput(Container::template make<T>(), label, true, false);
    }
    template <typename Container, typename T>
    Output* addOutput(const std::string& label) {
        Container::template registerType<T>();
        return addOutput(Container::template make<T>(), label, false);
    }
    template <typename Container, typename T>
    Output* addDynamicOutput(const std::string& label) {
        Container::template registerType<T>();
        return addOutput(Container::template make<T>(), label, true);
    }



    /// "direct" messages
    template <typename T>
    Input* addInput(const std::string& label,
                    typename std::enable_if<connection_types::should_use_pointer_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericPointerMessage<T>),
                      "connection_types::GenericPointerMessage is not included: "
                      "#include <csapex/msg/generic_pointer_message.hpp>");
        connection_types::MessageConversionHook<connection_types::GenericPointerMessage, T>::registerConversion();
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericPointerMessage<T> >(), label, false, false);
    }
    template <typename T>
    Input* addOptionalInput(const std::string& label,
                            typename std::enable_if<connection_types::should_use_pointer_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericPointerMessage<T>),
                      "connection_types::GenericPointerMessage is not included: "
                      "#include <csapex/msg/generic_pointer_message.hpp>");
        connection_types::MessageConversionHook<connection_types::GenericPointerMessage, T>::registerConversion();
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericPointerMessage<T> >(), label, false, true);
    }
    template <typename T>
    Input* addDynamicInput(const std::string& label,
                           typename std::enable_if<connection_types::should_use_pointer_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericPointerMessage<T>),
                      "connection_types::GenericPointerMessage is not included: "
                      "#include <csapex/msg/generic_pointer_message.hpp>");
        connection_types::MessageConversionHook<connection_types::GenericPointerMessage, T>::registerConversion();
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericPointerMessage<T> >(), label, true, false);
    }
    template <typename T>
    Output* addOutput(const std::string& label,
                      typename std::enable_if<connection_types::should_use_pointer_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericPointerMessage<T>),
                      "connection_types::GenericPointerMessage is not included: "
                      "#include <csapex/msg/generic_pointer_message.hpp>");
        connection_types::MessageConversionHook<connection_types::GenericPointerMessage, T>::registerConversion();
        return addOutput(connection_types::makeEmptyMessage<connection_types::GenericPointerMessage<T> >(), label, false);
    }
    template <typename T>
    Output* addDynamicOutput(const std::string& label,
                      typename std::enable_if<connection_types::should_use_pointer_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericPointerMessage<T>),
                      "connection_types::GenericPointerMessage is not included: "
                      "#include <csapex/msg/generic_pointer_message.hpp>");
        connection_types::MessageConversionHook<connection_types::GenericPointerMessage, T>::registerConversion();
        return addOutput(connection_types::makeEmptyMessage<connection_types::GenericPointerMessage<T> >(), label, true);
    }


    template <typename T>
    Input* addInput(const std::string& label,
                    typename std::enable_if<connection_types::should_use_value_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericValueMessage<T>),
                      "connection_types::GenericValueMessage is not included: "
                      "#include <csapex/msg/generic_value_message.hpp>");
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericValueMessage<T> >(), label, false, false);
    }
    template <typename T>
    Input* addDynamicInput(const std::string& label,
                           typename std::enable_if<connection_types::should_use_value_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericValueMessage<T>),
                      "connection_types::GenericValueMessage is not included: "
                      "#include <csapex/msg/generic_value_message.hpp>");
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericValueMessage<T> >(), label, true, false);
    }
    template <typename T>
    Input* addOptionalInput(const std::string& label,
                            typename std::enable_if<connection_types::should_use_value_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericValueMessage<T>),
                      "connection_types::GenericValueMessage is not included: "
                      "#include <csapex/msg/generic_value_message.hpp>");
        return addInput(connection_types::makeEmptyMessage<connection_types::GenericValueMessage<T> >(), label, false, true);
    }
    template <typename T>
    Output* addOutput(const std::string& label,
                      typename std::enable_if<connection_types::should_use_value_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericValueMessage<T>),
                      "connection_types::GenericValueMessage is not included: "
                      "#include <csapex/msg/generic_value_message.hpp>");
        return addOutput(connection_types::makeEmptyMessage<connection_types::GenericValueMessage<T> >(), label, false);
    }
    template <typename T>
    Output* addDynamicOutput(const std::string& label,
                             typename std::enable_if<connection_types::should_use_value_message<T>::value >::type* = 0) {
        static_assert(IS_COMPLETE(connection_types::GenericValueMessage<T>),
                      "connection_types::GenericValueMessage is not included: "
                      "#include <csapex/msg/generic_value_message.hpp>");
        return addOutput(connection_types::makeEmptyMessage<connection_types::GenericValueMessage<T> >(), label, true);
    }


    /// multiple input types allowed
    template <typename... Types>
    Input* addMultiInput(const std::string& label) {
        return addInput(multi_type::make<Types...>(), label, false, false);
    }

    template <typename... Types>
    Input* addOptionalMultiInput(const std::string& label) {
        return addInput(multi_type::make<Types...>(), label, false, true);
    }

    virtual bool isParameterInput(Input* in) const = 0;
    virtual bool isParameterOutput(Output* in) const = 0;


    /*
     * SIGNALING
     */
    Slot* addActiveSlot(const std::string& label, std::function<void()> callback);
    Slot* addActiveTypedSlot(const std::string& label, std::function<void(const TokenConstPtr&)> callback);
    Slot* addSlot(const std::string& label, std::function<void()> callback);
    Slot* addTypedSlot(const std::string& label, std::function<void(const TokenConstPtr&)> callback);
    virtual Slot* addSlot(const std::string& label, std::function<void (const TokenConstPtr& )> callback, bool active) = 0;

    virtual Event* addEvent(const std::string& label) = 0;


    std::vector<Input*> getMessageInputs() const;
    std::vector<Output*> getMessageOutputs() const;
    std::vector<Slot*> getSlots() const;
    std::vector<Event*> getEvents() const;

    virtual void removeInput(const UUID& uuid) = 0;
    virtual void removeOutput(const UUID& uuid) = 0;
    virtual void removeEvent(const UUID& uuid) = 0;
    virtual void removeSlot(const UUID& uuid) = 0;


    /*
     * MISCELLANEOUS
     */

    virtual bool isSource() const = 0;
    virtual void setIsSource(bool source) = 0;

    virtual bool isSink() const = 0;
    virtual void setIsSink(bool sink) = 0;

    bool isProcessingEnabled() const;
    void setProcessingEnabled(bool enabled);

    bool isError() const;
    void setNoError();
    void setError(const std::string& msg);
    void setWarning(const std::string& msg);

    NodeWorker* getNodeWorker() const;

    /**
     * Raw construction, handle with care!
     */
    virtual Input* addInput(TokenConstPtr type, const std::string& label, bool dynamic, bool optional) = 0;
    virtual Output* addOutput(TokenConstPtr type, const std::string& label, bool dynamic) = 0;

protected:
    virtual std::vector<ConnectablePtr> getAllConnectors() const = 0;
    virtual std::vector<InputPtr> getAllInputs() const = 0;
    virtual std::vector<OutputPtr> getAllOutputs() const = 0;
    virtual std::vector<SlotPtr> getAllSlots() const = 0;
    virtual std::vector<EventPtr> getAllEvents() const = 0;

private:
    mutable NodeWorker* node_worker_;
};

}

#endif // NODE_MODIFIER_H
