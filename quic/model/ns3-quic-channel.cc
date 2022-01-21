#include <string.h>
#include <string>
#include <iostream>
#include "gquiche/quic/platform/api/quic_logging.h"

#include "ns3-quic-channel.h"
namespace quic{
const char *hello="hello world";
void Ns3QuicChannelBase::set_notifier(Notifier *notifier){
    notifier_=notifier;
}
Ns3QuicHelloChannel::Ns3QuicHelloChannel(Ns3TransportStream *stream):
stream_(stream){}
Ns3QuicHelloChannel::~Ns3QuicHelloChannel()=default;

void Ns3QuicHelloChannel::SendHello(){
    //absl::string_view view(hello,strlen(hello));
    QUICHE_CHECK(stream_);
    stream_->Write(hello,strlen(hello),false);
}
void Ns3QuicHelloChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    stream_->SendFin();
    std::cout<<"Ns3QuicHelloChannel::read "<<buffer<<std::endl;
}
void Ns3QuicHelloChannel::OnCanWrite(){
    
}
void Ns3QuicHelloChannel::OnDestroy(){
    std::cout<<"Ns3QuicHelloChannel::OnDestroy"<<std::endl;
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}
Ns3QuicEchoOnceChannel::Ns3QuicEchoOnceChannel(Ns3TransportStream *stream):
stream_(stream){}
Ns3QuicEchoOnceChannel::~Ns3QuicEchoOnceChannel()=default;

void Ns3QuicEchoOnceChannel::OnCanRead(){
    std::string buffer;
    bool fin=false;
    stream_->Read(&buffer,fin);
    stream_->Write(buffer.data(),buffer.size(),true);
    std::cout<<"Ns3QuicEchoOnceChannel::read: "<<buffer<<std::endl;
}
void Ns3QuicEchoOnceChannel::OnCanWrite(){
    
}
void Ns3QuicEchoOnceChannel::OnDestroy(){
    std::cout<<"Ns3QuicEchoOnceChannel::OnDestroy"<<std::endl;
    if(notifier_){
        notifier_->OnChannelDestroy(this);
    }
}
}
