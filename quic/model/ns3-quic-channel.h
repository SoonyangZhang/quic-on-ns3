#pragma once
#include <memory>
#include "gquiche/quic/core/quic_alarm.h"
#include "gquiche/quic/core/quic_alarm_factory.h"
#include "ns3-transport-stream.h"
#include "ns3/ns3-quic-public.h"
namespace quic{
class Ns3QuicAlarmEngine;
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
class HelloBidiChannel:public Ns3QuicChannelBase{
public:
    HelloBidiChannel(Ns3TransportStream *stream);
    ~HelloBidiChannel() override{}
    void SendHello();
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
    
};
class EchoBidiChannel:public Ns3QuicChannelBase{
public:
    EchoBidiChannel(Ns3TransportStream *stream);
    ~EchoBidiChannel() override{}
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
};
class HelloUnidiWriteChannel:public Ns3QuicChannelBase{
public:
    HelloUnidiWriteChannel(Ns3TransportStream *stream);
    ~HelloUnidiWriteChannel() override{}
    void SendHello();
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
};
class HelloUnidiReadChannel:public Ns3QuicChannelBase{
public:
    HelloUnidiReadChannel(Ns3TransportStream *stream);
    ~HelloUnidiReadChannel() override{}
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
};

class BandwidthWriteChannel:public Ns3QuicChannelBase{
public:
    BandwidthWriteChannel(Ns3TransportStream *stream,Ns3QuicAlarmEngine *engine);
    ~BandwidthWriteChannel() override;
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
    void OnTimeout();
    void StartFillData();
private:
    void WriteData();
    std::unique_ptr<QuicAlarmFactory> alarm_factory_;
    std::unique_ptr<QuicAlarm> alarm_;
    Ns3TransportStream *stream_=nullptr;
    uint32_t write_bytes_=0;
};
class BandwidthReadChannel:public Ns3QuicChannelBase{
public:
    BandwidthReadChannel(Ns3TransportStream *stream);
    ~BandwidthReadChannel() override;
    //from Ns3TransportStream::Visitor
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnDestroy() override;
private:
    Ns3TransportStream *stream_=nullptr;
    uint32_t read_bytes_=0;
};

}
