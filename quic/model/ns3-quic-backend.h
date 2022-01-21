#pragma once
#include <memory>
#include "ns3/ns3-quic-channel.h"
namespace quic{
class Ns3TransportStream;
class Ns3QuicSessionBase;
class Ns3QuicAlarmEngine;
class Ns3QuicEndpointBase{
public:
    virtual ~Ns3QuicEndpointBase(){}
    virtual void OnIncomingBidirectionalStream(Ns3TransportStream *stream)=0;
    virtual void OnIncomingUnidirectionalStream(Ns3TransportStream *stream)=0;
    virtual void OnSessionReady(Ns3QuicSessionBase *session)=0;
    virtual void OnSessionDestroy(Ns3QuicSessionBase *session)=0;
};
class Ns3QuicBackendBase{
public:
    virtual ~Ns3QuicBackendBase(){}
    virtual Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session)=0;
};
class HelloClientBackend:public Ns3QuicBackendBase{
public:
    HelloClientBackend(Ns3QuicAlarmEngine* engine);
    ~HelloClientBackend() override;
    Ns3QuicAlarmEngine* get_engine();
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
private:
    Ns3QuicAlarmEngine *engine_=nullptr;
};
class EchoServerBackend:public Ns3QuicBackendBase{
public:
    EchoServerBackend(Ns3QuicAlarmEngine* engine);
    ~EchoServerBackend() override;
    Ns3QuicAlarmEngine* get_engine();
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
private:
    Ns3QuicAlarmEngine *engine_=nullptr;
};

class HelloClientEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    HelloClientEndpoint(HelloClientBackend* backend);
    ~HelloClientEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    HelloClientBackend *backend_=nullptr;
    Ns3QuicSessionBase *session_=nullptr;
    bool ready_=false;
};
class EchoServerEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    EchoServerEndpoint(EchoServerBackend* backend);
    ~EchoServerEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    EchoServerBackend *backend_=nullptr;
    Ns3QuicSessionBase *session_=nullptr;
    bool ready_=false;
};
}
