#pragma once
#include "ns3/ns3-transport-stream.h"
namespace quic{
class Ns3QuicChannelBase:public Ns3TransportStream::Visitor{
public:
    class Notifier{
    public:
        virtual ~Notifier(){}
        virtual void OnChannelDestroy(Ns3QuicChannelBase *channel)=0;
    };
    void set_notifier(Notifier *notifier);
    virtual ~Ns3QuicChannelBase(){}
protected:
    Notifier *notifier_=nullptr;
};
class Ns3QuicHelloChannel:public Ns3QuicChannelBase{
public:
    Ns3QuicHelloChannel(Ns3TransportStream *stream);
    ~Ns3QuicHelloChannel() override;
    void SendHello();
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
    
};
class Ns3QuicEchoOnceChannel:public Ns3QuicChannelBase{
public:
    Ns3QuicEchoOnceChannel(Ns3TransportStream *stream);
    ~Ns3QuicEchoOnceChannel() override;
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
};
}
