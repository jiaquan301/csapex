/// HEADER
#include <csapex/view/csapex_view_core_remote.h>


/// COMPONENT
#include <csapex/core/csapex_core.h>
#include <csapex/view/utility/message_renderer_manager.h>
#include <csapex/view/node/node_adapter_factory.h>
#include <csapex/view/designer/drag_io.h>
#include <csapex/model/graph_facade.h>
#include <csapex/scheduling/thread_pool.h>
#include <csapex/command/dispatcher.h>
#include <csapex/io/session.h>
#include <csapex/io/protcol/core_requests.h>
#include <csapex/io/broadcast_message.h>
#include <csapex/io/protcol/notification_message.h>
#include <csapex/serialization/packet_serializer.h>
#include <csapex/io/protcol/parameter_changed.h>

/// SYSTEM
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/version.hpp>
#if (BOOST_VERSION / 100000) >= 1 && (BOOST_VERSION / 100 % 1000) >= 54
namespace bf3 = boost::filesystem;
#else
namespace bf3 = boost::filesystem3;
#endif

using boost::asio::ip::tcp;
using namespace csapex;


CsApexViewCoreRemote::CsApexViewCoreRemote(const std::string &ip, int port, CsApexCorePtr core_tmp)
    : socket(io_service),
      resolver(io_service),
      resolver_iterator(boost::asio::connect(socket, resolver.resolve({ip, std::to_string(port)}))),
      session_(std::make_shared<Session>(std::move(socket))),
      settings_(std::make_shared<SettingsRemote>(session_)),
      node_adapter_factory_(std::make_shared<NodeAdapterFactory>(*settings_, core_tmp->getPluginLocator().get())),

      core_tmp_(core_tmp)
{
    session_->start();

    dispatcher_ = core_tmp_->getCommandDispatcher();
    node_factory_ = core_tmp_->getNodeFactory();
    snippet_factory_ = core_tmp_->getSnippetFactory();

    session_->packet_received.connect([this](SerializableConstPtr packet){
        handlePacket(packet);
    });

    running = true;
    spinner = std::thread([&](){
        while(running) {
            io_service.run();
        }
    });
}

CsApexViewCoreRemote::~CsApexViewCoreRemote()
{
    if(spinner.joinable()) {
        spinner.join();
    }
}


void CsApexViewCoreRemote::handlePacket(SerializableConstPtr packet)
{
    if(packet) {
        //                std::cout << "type=" << (int) serial->getPacketType() << std::endl;

        switch(packet->getPacketType()) {
        case BroadcastMessage::PACKET_TYPE_ID:
            if(BroadcastMessageConstPtr broadcast = std::dynamic_pointer_cast<BroadcastMessage const>(packet)) {
                if(auto notification_msg = std::dynamic_pointer_cast<NotificationMessage const>(broadcast)) {
                    Notification n = notification_msg->getNotification();
                    notification(n);
                }
            }
        }
    }
}


NodeAdapterFactoryPtr CsApexViewCoreRemote::getNodeAdapterFactory()
{
    return node_adapter_factory_;
}

std::shared_ptr<DragIO> CsApexViewCoreRemote::getDragIO()
{
    return drag_io;
}

/// PROXIES
ExceptionHandler& CsApexViewCoreRemote::getExceptionHandler() const
{
    return core_tmp_->getExceptionHandler();
}


PluginLocatorPtr CsApexViewCoreRemote::getPluginLocator() const
{
    return core_tmp_->getPluginLocator();
}

CommandExecutorPtr CsApexViewCoreRemote::getCommandDispatcher()
{
    return dispatcher_;
}

Settings& CsApexViewCoreRemote::getSettings() const
{
    return *settings_;
}


GraphFacadePtr CsApexViewCoreRemote::getRoot()
{
    return core_tmp_->getRoot();
}

ThreadPoolPtr CsApexViewCoreRemote::getThreadPool()
{
    // TODO: replace with proxy
    //apex_assert_hard(//core_->getThreadPool());
    return core_tmp_->getThreadPool();
}
NodeFactoryPtr CsApexViewCoreRemote::getNodeFactory() const
{
    // TODO: replace with proxy
    apex_assert_hard(node_factory_);
    return node_factory_;
}
SnippetFactoryPtr CsApexViewCoreRemote::getSnippetFactory() const
{
    // TODO: replace with proxy
    apex_assert_hard(snippet_factory_);
    return snippet_factory_;
}
ProfilerPtr CsApexViewCoreRemote::getProfiler() const
{
    // TODO: replace with proxy
    //apex_assert_hard(//core_->getProfiler() != nullptr);
    return core_tmp_->getProfiler();
}

void CsApexViewCoreRemote::sendNotification(const std::string& notification, ErrorState::ErrorLevel error_level)
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreSendNotification, notification, static_cast<uint8_t>(error_level));
}



/// RELAYS

void CsApexViewCoreRemote::reset()
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreReset);
}


void CsApexViewCoreRemote::load(const std::string& file)
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreLoad, file);
}

void CsApexViewCoreRemote::saveAs(const std::string& file, bool quiet)
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreSave, file, quiet);
}

bool CsApexViewCoreRemote::isPaused() const
{
    auto res = session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreGetPause);
    apex_assert_hard(res);
    return res->getResult<bool>();
}

void CsApexViewCoreRemote::setPause(bool paused)
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreSetPause, paused);
}


bool CsApexViewCoreRemote::isSteppingMode() const
{
    auto res = session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreGetSteppingMode);
    apex_assert_hard(res);
    return res->getResult<bool>();
}

void CsApexViewCoreRemote::setSteppingMode(bool stepping)
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreSetSteppingMode, stepping);
}

void CsApexViewCoreRemote::step()
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreStep);
}


void CsApexViewCoreRemote::shutdown()
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreShutdown);
    running = false;
    io_service.stop();
}

void CsApexViewCoreRemote::clearBlock()
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreClearBlock);
}

void CsApexViewCoreRemote::resetActivity()
{
    session_->sendRequest<CoreRequests>(CoreRequests::CoreRequestType::CoreResetActivity);
}
