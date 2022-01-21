#include <iostream>
#include <memory>
#include "gquiche/quic/core/quic_constants.h"
#include "gquiche/quic/platform/api/quic_default_proof_providers.h"
#include "gquiche/quic/platform/api/quic_logging.h"

#include "ns3/quic-test.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3-quic-util.h"
#include "ns3-quic-flags.h"
namespace quic{
void set_test_value(const std::string &v){
    std::string key("test_value");
    ns3::set_quic_key_value(key,v);
}
void print_test_value(){
    int32_t test_value=get_test_value();
    std::cout<<test_value<<std::endl;
}
void parser_flags(const char* usage,int argc, const char* const* argv){
    quic::QuicParseCommandLineFlags(usage, argc, argv);
}
class AlarmTestDelegate:public QuicAlarm::Delegate{
public:
    AlarmTestDelegate(AlarmTest *entity):entity_(entity){}
    void OnAlarm() override{
        entity_->OnTimeout();
    }
private:
    AlarmTest *entity_;
};
AlarmTest::AlarmTest(ns3::Time start,ns3::Time stop,ns3::Time gap,int max_v):
    clock_(Ns3QuicClock::Instance()),
    engine_(ns3::CreateObject<Ns3QuicAlarmEngine>()),
    alarm_fatory_(new Ns3AlarmFactory(ns3::GetPointer(engine_))),
    alarm_(alarm_fatory_->CreateAlarm(
                                arena_.New<AlarmTestDelegate>(this), &arena_)){

    start_timer_=ns3::Simulator::Schedule(start,&AlarmTest::StartApplication,this);
    stop_timer_=ns3::Simulator::Schedule(stop,&AlarmTest::StopApplication,this);
    interval_=gap;
    max_v_=max_v;
}
AlarmTest::~AlarmTest(){
    StopApplication();
}
void AlarmTest::StartApplication(){
    //NS_ASSERT(alarm_->IsSet()==false);
    alarm_->Set(clock_->Now());
}
void AlarmTest::StopApplication(){
    alarm_->Cancel();
}
void AlarmTest::OnTimeout(){
    ns3::Time now=ns3::Simulator::Now();
    if(count_<max_v_){
        QuicTime::Delta delay=QuicTime::Delta::FromMilliseconds(interval_.GetMilliSeconds());
        alarm_->Update(clock_->Now()+delay, kAlarmGranularity);
    }
    int delta=0;
    if(last_time_>0){
        delta=now.GetMilliSeconds()-last_time_;
    }
    std::cout<<count_<<" "<<delta<<std::endl;
    last_time_=now.GetMilliSeconds();
    count_++;
}
void print_address(){
    {
        uint16_t port=1234;
        ns3::InetSocketAddress ns3_socket_addr("10.0.1.1",port);
        quic::QuicSocketAddress quic_socket_addr=ns3::GetQuicSocketAddr(ns3_socket_addr);
        std::cout<<quic_socket_addr.ToString()<<std::endl;
    }
    {
        uint16_t port=5678;
        quic::QuicIpAddress quic_ip_addr;
        quic_ip_addr.FromString("10.0.2.1");
        quic::QuicSocketAddress quic_socket_addr(quic_ip_addr,port);
        ns3::InetSocketAddress ns3_socket_addr=ns3::GetNs3SocketAddr(quic_socket_addr);
        std::cout<<ns3_socket_addr.GetIpv4()<<":"<<ns3_socket_addr.GetPort()<<std::endl;
    }
}
void test_quic_log(){
    std::cout<<quiche::IsLogLevelEnabled(quiche::INFO)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::WARNING)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::ERROR)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::FATAL)<<std::endl;
    QUIC_LOG(INFO)<<"hello logger info";
    QUIC_LOG(ERROR)<<"hello logger error";
    QUIC_DLOG(FATAL) << "Unable to read key.";
    QUICHE_CHECK(0);
}
void test_proof_source(){
    std::unique_ptr<quic::ProofSource> proof_source=quic::CreateDefaultProofSource();
    if(!proof_source){
        QUIC_LOG(ERROR)<<"quic_cert path is not right"<<std::endl;
    }
}
}
