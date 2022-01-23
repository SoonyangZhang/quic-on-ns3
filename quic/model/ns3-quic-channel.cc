#include <string.h>
#include <string>
#include "gquiche/quic/platform/api/quic_logging.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3-quic-channel.h"
#include "ns3-quic-alarm-engine.h"
#include "ns3-quic-clock.h"
NS_LOG_COMPONENT_DEFINE("Ns3QuicChannelBase");
namespace quic{
namespace{
    const char *hello="hello world";
    char kContent[1500]={0};
    int kWritePackets=6;
    QuicTime::Delta kWriteDelta=QuicTime::Delta::FromMilliseconds(50);
}
void ContentInit(const char v){
    int len=sizeof(kContent)/sizeof(char);
    for(int i=0;i<len;i++){
        kContent[i]=v;
    }
}
void Ns3QuicChannelBase::set_notifier(Notifier *notifier){
    notifier_=notifier;
}
HelloBidiChannel::HelloBidiChannel(Ns3TransportStream *stream):
stream_(stream){}

void HelloBidiChannel::SendHello(){
    QUICHE_CHECK(stream_);
    stream_->Write(hello,strlen(hello),true);
}
void HelloBidiChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    NS_LOG_INFO("HelloBidiChannel::read "<<buffer);
}
void HelloBidiChannel::OnCanWrite(){}
void HelloBidiChannel::OnDestroy(){
    NS_LOG_INFO("HelloBidiChannel::OnDestroy");
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

EchoBidiChannel::EchoBidiChannel(Ns3TransportStream *stream):
stream_(stream){}

void EchoBidiChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    stream_->Write(buffer.data(),buffer.size(),true);
    NS_LOG_INFO("EchoBidiChannel::read "<<buffer);
}
void EchoBidiChannel::OnCanWrite(){}
void EchoBidiChannel::OnDestroy(){
    NS_LOG_INFO("EchoBidiChannel::OnDestroy");
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

HelloUnidiWriteChannel::HelloUnidiWriteChannel(Ns3TransportStream *stream):
stream_(stream){}
    
void HelloUnidiWriteChannel::SendHello(){
    stream_->Write(hello,strlen(hello),true);
}
void HelloUnidiWriteChannel::OnCanRead(){}
void HelloUnidiWriteChannel::OnCanWrite(){
    NS_LOG_INFO("HelloUnidiWriteChannel::OnCanWrite");
}
void HelloUnidiWriteChannel::OnDestroy(){
    NS_LOG_INFO("HelloUnidiWriteChannel::OnDestroy");
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

HelloUnidiReadChannel::HelloUnidiReadChannel(Ns3TransportStream *stream):
stream_(stream){}

void HelloUnidiReadChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    NS_LOG_INFO("HelloUnidiReadChannel::read "<<buffer);
}
void HelloUnidiReadChannel::OnCanWrite(){}
void HelloUnidiReadChannel::OnDestroy(){
    NS_LOG_INFO("HelloUnidiReadChannel::OnDestroy");
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

class WriteDelegate:public QuicAlarm::Delegate{
public:
    WriteDelegate(BandwidthWriteChannel *entity):entity_(entity){}
    ~WriteDelegate() override{}
    //from QuicAlarm::Delegate
    void OnAlarm() override{
        entity_->OnTimeout();
    }
private:
    BandwidthWriteChannel *entity_;
};
BandwidthWriteChannel::BandwidthWriteChannel(Ns3TransportStream *stream,Ns3QuicAlarmEngine *engine):
stream_(stream){
    alarm_factory_.reset(new Ns3AlarmFactory(engine));
    alarm_.reset(alarm_factory_->CreateAlarm(new WriteDelegate(this)));
}
BandwidthWriteChannel::~BandwidthWriteChannel(){
    if(alarm_&&alarm_->IsSet()){
        alarm_->Cancel();
    }
    alarm_=nullptr;
    alarm_factory_=nullptr;
    NS_LOG_INFO("BandwidthWriteChannel::dtor "<<write_bytes_);
}
void BandwidthWriteChannel::OnCanRead(){}
void BandwidthWriteChannel::OnCanWrite(){
    WriteData();
    NS_LOG_INFO("BandwidthWriteChannel::OnCanWrite");
}
void BandwidthWriteChannel::OnDestroy(){
    NS_LOG_INFO("BandwidthWriteChannel::OnDestroy");
    if(alarm_&&alarm_->IsSet()){
        alarm_->Cancel();
    }
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

void BandwidthWriteChannel::OnTimeout(){
    QuicTime now=Ns3QuicClock::Instance()->Now();
    WriteData();
    alarm_->Update(now+kWriteDelta,kAlarmGranularity);
}
void BandwidthWriteChannel::StartFillData(){
    WriteData();
    QuicTime now=Ns3QuicClock::Instance()->Now();
    alarm_->Set(now+kWriteDelta);
}
void BandwidthWriteChannel::WriteData(){
    int couter=0;
    QuicTime now=Ns3QuicClock::Instance()->Now();   
    if(stream_){
        for(int i=0;i<kWritePackets;i++){
            bool success=stream_->Write(kContent,1500,false);
            if(!success){
                break;
            }
            couter++;
            write_bytes_+=1500;
        }
    }
    float seconds=(now-QuicTime::Zero()).ToMilliseconds()*1.0/1000;
    NS_LOG_INFO(seconds<<" BandwidthWriteChannel::WriteData "<<couter);
}

BandwidthReadChannel::BandwidthReadChannel(Ns3TransportStream *stream):
stream_(stream){}
BandwidthReadChannel::~BandwidthReadChannel(){
    NS_LOG_INFO("BandwidthReadChannel::dtor "<<read_bytes_);
}
void BandwidthReadChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    read_bytes_+=buffer.size();
}
void BandwidthReadChannel::OnCanWrite(){}
void BandwidthReadChannel::OnDestroy(){
    NS_LOG_INFO("BandwidthReadChannel::OnDestroy");
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}

}


