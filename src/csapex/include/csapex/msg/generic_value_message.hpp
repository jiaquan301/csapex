#ifndef GENERIC_VALUE_MESSAGE_H
#define GENERIC_VALUE_MESSAGE_H

/// COMPONENT
#include <csapex/msg/message.h>
#include <csapex/utility/register_msg.h>

namespace csapex {
namespace connection_types {

template <typename Type>
struct GenericValueMessage : public Message
{
    typedef boost::shared_ptr<GenericValueMessage<Type> > Ptr;
    typedef boost::shared_ptr<GenericValueMessage<Type> const> ConstPtr;

    GenericValueMessage(const std::string& frame_id = "/", Message::Stamp stamp = 0)
        : Message(type< GenericValueMessage<Type> >::name(), frame_id, stamp)
    {
        static csapex::MessageRegistered<GenericValueMessage<Type> > reg;
    }

    virtual ConnectionType::Ptr clone() const override
    {
        Ptr new_msg(new GenericValueMessage<Type>(frame_id, stamp));
        new_msg->value = value;
        return new_msg;
    }

    virtual ConnectionType::Ptr toType() const override
    {
        Ptr new_msg(new GenericValueMessage<Type>("/"));
        return new_msg;
    }

    bool acceptsConnectionFrom(const ConnectionType* other_side) const override
    {
        return name() == other_side->name();
    }

    Type getValue()
    {
        return value;
    }

    Type value;
};


/// TRAITS
template <typename T>
struct type<GenericValueMessage<T> > {
    static std::string name() {
        return std::string("Value<") + type2name(typeid(T)) + ">";
    }
};

}
}

/// YAML
namespace YAML {
template<typename T>
struct convert<csapex::connection_types::GenericValueMessage<T> >
{
  static Node encode(const csapex::connection_types::GenericValueMessage<T>& rhs)
  {
      Node node;
      node["value"] = rhs.value;
      return node;
  }

  static bool decode(const Node& node, csapex::connection_types::GenericValueMessage<T>& rhs)
  {
      if(!node.IsMap()) {
          return false;
      }

      rhs.value = node["value"].as<T>();
      return true;
  }
};
}


#endif // GENERIC_VALUE_MESSAGE_H
