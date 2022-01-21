#pragma once
#include <memory>
#include <string>
#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/ns3-quic-clock.h"
#include "ns3/ns3-quic-alarm-engine.h"
#include "gquiche/quic/core/quic_one_block_arena.h"
namespace quic{
void set_test_value(const std::string &v);
void print_test_value();
void print_address();
void parser_flags(const char* usage,int argc, const char* const* argv);
void test_quic_log();
void test_proof_source();
class AlarmTest:public ns3::Object{
public:
    AlarmTest(ns3::Time start,ns3::Time stop,ns3::Time gap,int max_v);
    ~AlarmTest();
    void StartApplication();
    void StopApplication();
    void OnTimeout();
private:
    QuicClock *clock_=nullptr;
    ns3::Ptr<Ns3QuicAlarmEngine> engine_;
    std::unique_ptr<Ns3AlarmFactory> alarm_fatory_;
    QuicArenaScopedPtr<QuicAlarm> alarm_;
    QuicConnectionArena arena_;
    ns3::EventId start_timer_;
    ns3::EventId stop_timer_;
    ns3::Time interval_;
    int64_t last_time_=-1;
    int max_v_=0;
    int count_=0;
};
}
