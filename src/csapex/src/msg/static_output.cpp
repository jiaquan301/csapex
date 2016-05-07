/// HEADER
#include <csapex/msg/static_output.h>

/// COMPONENT
#include <csapex/msg/message.h>
#include <csapex/msg/input.h>
#include <csapex/model/connection.h>
#include <csapex/utility/assert.h>
#include <csapex/msg/output_transition.h>
#include <csapex/msg/no_message.h>

/// SYSTEM
#include <iostream>

using namespace csapex;

StaticOutput::StaticOutput(const UUID &uuid)
    : Output(uuid), message_flags_(0)
{

}

void StaticOutput::addMessage(Token::ConstPtr message)
{
    setType(message->toType());

    // update buffer

    apex_assert_hard(message != nullptr);
    message_to_send_ = message;
}

bool StaticOutput::hasMessage()
{
    return (bool) message_to_send_;
}


bool StaticOutput::hasMarkerMessage()
{
    if(auto m = std::dynamic_pointer_cast<connection_types::MarkerMessage const>(message_to_send_)) {
        if(!std::dynamic_pointer_cast<connection_types::NoMessage const>(m)) {
            return true;
        }
    }

    return false;
}


void StaticOutput::nextMessage()
{
    setState(State::IDLE);
}

TokenConstPtr StaticOutput::getMessage() const
{
    if(!committed_message_) {
        return connection_types::makeEmpty<connection_types::NoMessage>();
    } else {
        return committed_message_;
    }
}

void StaticOutput::setMultipart(bool multipart, bool last_part)
{
    message_flags_ = 0;
    if(multipart) {
        message_flags_ |= (int) Token::Flags::Fields::MULTI_PART;
        if(last_part) {
            message_flags_ |= (int) Token::Flags::Fields::LAST_PART;
        }
    }
}

void StaticOutput::commitMessages(bool is_activated)
{
    apex_assert_hard(canSendMessages());

    activate();


    if(message_to_send_) {
        committed_message_ = message_to_send_;
        clearBuffer();

    } else {
        if(!connections_.empty()) {
//            std::cout << getUUID() << " sends empty message" << std::endl;
        }
        committed_message_ = connection_types::makeEmpty<connection_types::NoMessage>();
    }

    ++seq_no_;
    committed_message_->setSequenceNumber(seq_no_);
    committed_message_->flags.data = message_flags_;

    committed_message_->setActive(is_activated);

    ++count_;
    messageSent(this);
}

void StaticOutput::reset()
{
    Output::reset();
    committed_message_.reset();
    message_to_send_.reset();
}

void StaticOutput::disable()
{
    Output::disable();

    message_to_send_.reset();
    committed_message_.reset();
}

Token::ConstPtr StaticOutput::getMessage()
{
    return committed_message_;
}

void StaticOutput::clearBuffer()
{
    message_to_send_.reset();
}
