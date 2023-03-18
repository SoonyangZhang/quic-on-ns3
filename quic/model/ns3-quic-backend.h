#pragma once
#include <memory>
#include "ns3/ns3-quic-public.h"
#include "ns3-quic-channel.h"
namespace quic{
class Ns3TransportStream;
class Ns3QuicSessionBase;
class Ns3QuicAlarmEngine;
class Ns3QuicBackendBase;
class Ns3QuicEndpointBase{
public:
    Ns3QuicEndpointBase(Ns3QuicBackendBase* backend);
    virtual ~Ns3QuicEndpointBase(){}
    virtual void OnIncomingBidirectionalStream(Ns3TransportStream *stream)=0;
    virtual void OnIncomingUnidirectionalStream(Ns3TransportStream *stream)=0;
    virtual void OnSessionReady(Ns3QuicSessionBase *session)=0;
    virtual void OnSessionDestroy(Ns3QuicSessionBase *session)=0;
protected:
    Ns3QuicBackendBase *backend_=nullptr;
    Ns3QuicSessionBase *session_=nullptr;
    bool ready_=false;
};
class Ns3QuicBackendBase{
public:
    Ns3QuicBackendBase(Ns3QuicAlarmEngine* engine);
    virtual ~Ns3QuicBackendBase(){}
    virtual Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session)=0;
    Ns3QuicAlarmEngine* get_engine() {return engine_;}
protected:
    Ns3QuicAlarmEngine *engine_=nullptr;
};

class HelloClientEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    HelloClientEndpoint(Ns3QuicBackendBase* backend,bool bidi_channel);
    ~HelloClientEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    Ns3QuicChannelBase *channel_=nullptr;
    bool bidi_channel_=false;
};
class EchoServerEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    EchoServerEndpoint(Ns3QuicBackendBase* backend);
    ~EchoServerEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    Ns3QuicChannelBase *channel_=nullptr;
};

class HelloClientBackend:public Ns3QuicBackendBase{
public:
    HelloClientBackend(Ns3QuicAlarmEngine* engine,bool bidi_channel=true);
    ~HelloClientBackend() override{}
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
private:
    bool bidi_channel_=true;
};
class HelloServerBackend:public Ns3QuicBackendBase{
public:
    HelloServerBackend(Ns3QuicAlarmEngine* engine);
    ~HelloServerBackend() override {}
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
};

class BandwidthClientEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    BandwidthClientEndpoint(Ns3QuicBackendBase* backend);
    ~BandwidthClientEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    Ns3QuicChannelBase *channel_=nullptr;
};

class BandwidthServerEndpoint:public Ns3QuicEndpointBase,
public Ns3QuicChannelBase::Notifier{
public:
    BandwidthServerEndpoint(Ns3QuicBackendBase* backend);
    ~BandwidthServerEndpoint() override;
    //from Ns3QuicEndpointBase
    void OnIncomingBidirectionalStream(Ns3TransportStream *stream) override;
    void OnIncomingUnidirectionalStream(Ns3TransportStream *stream) override;
    void OnSessionReady(Ns3QuicSessionBase *session) override;
    void OnSessionDestroy(Ns3QuicSessionBase *session) override;
    //from Ns3QuicChannelBase::Notifier
    void OnChannelDestroy(Ns3QuicChannelBase *channel) override;
private:
    Ns3QuicChannelBase *channel_=nullptr;
};

class BandwidthClientBackend:public Ns3QuicBackendBase{
public:
    BandwidthClientBackend(Ns3QuicAlarmEngine* engine);
    ~BandwidthClientBackend() override {}
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
};

class BandwidthServerBackend:public Ns3QuicBackendBase{
public:
    BandwidthServerBackend(Ns3QuicAlarmEngine* engine);
    ~BandwidthServerBackend() override {}
    Ns3QuicEndpointBase* CreateEndpoint(Ns3QuicSessionBase *session) override;
};
}
