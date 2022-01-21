#include <memory>
#include "gquiche/quic/platform/api/quic_logging.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3-quic-backend.h"
#include "ns3-quic-alarm-engine.h"
#include "ns3-quic-session-base.h"
#include "ns3-transport-stream.h"
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("Ns3QuicBackendBase");
namespace quic{
HelloClientBackend::HelloClientBackend(Ns3QuicAlarmEngine* engine):
engine_(engine){}
HelloClientBackend::~HelloClientBackend(){}

Ns3QuicEndpointBase* HelloClientBackend::CreateEndpoint(Ns3QuicSessionBase *session){
    return new HelloClientEndpoint(this);
}
Ns3QuicAlarmEngine* HelloClientBackend::get_engine(){
    return engine_;
}

EchoServerBackend::EchoServerBackend(Ns3QuicAlarmEngine* engine):
engine_(engine){}
EchoServerBackend::~EchoServerBackend(){}

Ns3QuicEndpointBase* EchoServerBackend::CreateEndpoint(Ns3QuicSessionBase *session){
    return new EchoServerEndpoint(this);
}
Ns3QuicAlarmEngine* EchoServerBackend::get_engine(){
    return engine_;
}

HelloClientEndpoint::HelloClientEndpoint(HelloClientBackend* backend):
backend_(backend){}
HelloClientEndpoint::~HelloClientEndpoint()=default;

void HelloClientEndpoint::OnIncomingBidirectionalStream(Ns3TransportStream *stream){
    
}
void HelloClientEndpoint::OnIncomingUnidirectionalStream(Ns3TransportStream *stream){
    
}
void HelloClientEndpoint::OnSessionReady(Ns3QuicSessionBase *session){
    ready_=true;
    session_=session;
    Ns3TransportStream *stream=session_->OpenOutgoingBidirectionalStream();
    QUICHE_CHECK(stream);;
    Ns3QuicHelloChannel *channel_po=new Ns3QuicHelloChannel(stream);
    std::unique_ptr<Ns3QuicHelloChannel> channel(channel_po);
    channel->set_notifier(this);
    stream->set_visitor(std::move(channel));
    channel_po->SendHello();
}
void HelloClientEndpoint::OnSessionDestroy(Ns3QuicSessionBase *session){
    NS_LOG_INFO("HelloClientEndpoint::OnSessionDestroy");
    session_=nullptr;
}
void HelloClientEndpoint::OnChannelDestroy(Ns3QuicChannelBase *channel){
    
}

EchoServerEndpoint::EchoServerEndpoint(EchoServerBackend* backend):
backend_(backend){}
EchoServerEndpoint::~EchoServerEndpoint()=default;

void EchoServerEndpoint::OnIncomingBidirectionalStream(Ns3TransportStream *stream){
    std::unique_ptr<Ns3QuicEchoOnceChannel> channel(new Ns3QuicEchoOnceChannel(stream));
    channel->set_notifier(this);
    stream->set_visitor(std::move(channel));
}
void EchoServerEndpoint::OnIncomingUnidirectionalStream(Ns3TransportStream *stream){
     QUICHE_CHECK(0);
}
void EchoServerEndpoint::OnSessionReady(Ns3QuicSessionBase *session){
    ready_=true;
    session_=session;
}
void EchoServerEndpoint::OnSessionDestroy(Ns3QuicSessionBase *session){
    NS_LOG_INFO("EchoServerEndpoint::OnSessionDestroy");
    session_=nullptr;
}
void EchoServerEndpoint::OnChannelDestroy(Ns3QuicChannelBase *channel){
    
}
}
