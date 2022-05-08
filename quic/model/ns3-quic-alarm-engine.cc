#include <iostream>
#include <string>
#include <limits>
#include <vector>

#include "gquiche/quic/platform/api/quic_logging.h"
#include "gquiche/quic/core/quic_arena_scoped_ptr.h"


#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3-quic-alarm-engine.h"
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("Ns3QuicAlarmEngine");
#define MOD_DEBUG 1
namespace quic{
Ns3QuicAlarmEngine::Ns3QuicAlarmEngine(Perspective role):
role_(role),
visitor_(nullptr){}
Ns3QuicAlarmEngine::~Ns3QuicAlarmEngine(){
    Shutdown();
}
void Ns3QuicAlarmEngine::Shutdown(){
    TimeToAlarmCBMap::iterator erase_it;
    for(auto i=alarm_map_.begin();i!=alarm_map_.end();){
        AlarmCB* cb = i->second;
        erase_it=i;
        i++;
        alarm_map_.erase(erase_it);
        all_alarms_.erase(cb);
        cb->OnShutdown(this);
    }
}
void Ns3QuicAlarmEngine::RegisterAlarm(int64_t timeout_us,AlarmCB* ac){
    bool update=false;
    ns3::Time now=ns3::Simulator::Now();
    int64_t now_us=now.GetMicroSeconds();
    if(timeout_us<now_us){
    #if (MOD_DEBUG)
        NS_LOG_FUNCTION(now_us<<timeout_us);
    #endif
        timeout_us=now_us;
    }
    if(alarm_map_.empty()){
        update=true;
    }else{
        auto i=alarm_map_.begin();
        int64_t next_timeout_us=i->first;
        if(timeout_us<next_timeout_us){
            update=true;
        }
    }
    if(all_alarms_.find(ac)==all_alarms_.end()){
        auto alarm_iter = alarm_map_.insert(std::make_pair(timeout_us, ac));
        all_alarms_.insert(ac);
        ac->OnRegistration(alarm_iter,this);
    }
    if((!timer_.IsRunning())||update){
        UpdateTimer();
    }

}
void Ns3QuicAlarmEngine::UnregisterAlarm(const AlarmRegToken & iterator_token){
    AlarmCB* cb = iterator_token->second;
    alarm_map_.erase(iterator_token);
    all_alarms_.erase(cb);
    cb->OnUnregistration();
}
Ns3QuicAlarmEngine::AlarmRegToken Ns3QuicAlarmEngine::ReregisterAlarm(AlarmRegToken &iterator_token, int64_t timeout_us){
  int64_t now_us=ns3::Simulator::Now().GetMicroSeconds();
  AlarmCB* cb = iterator_token->second;
  int64_t last_us=iterator_token->first;
  alarm_map_.erase(iterator_token);
  NS_ASSERT(!alarm_map_.empty());
  auto i=alarm_map_.begin();
  int64_t next_timeout_us=i->first;
  #if (MOD_DEBUG)
  if(timeout_us<=now_us){
      NS_LOG_FUNCTION(now_us<<timeout_us<<next_timeout_us<<last_us);
  }
  #endif
  auto ret=alarm_map_.emplace(timeout_us, cb);
  if((!timer_.IsRunning())||(timeout_us<next_timeout_us)){
       UpdateTimer();
  }
  NS_ASSERT_MSG(timer_.IsRunning(),now_us<<":"<<timeout_us
                <<":"<<next_timeout_us<<":"<<alarm_map_.size());
  return  ret;
}
void Ns3QuicAlarmEngine::UpdateTimer(){
    if(timer_.IsRunning()){
         timer_.Cancel();
    }
    if(alarm_map_.empty()){ return ;}
    auto i=alarm_map_.begin();
    int64_t now_us=ns3::Simulator::Now().GetMicroSeconds();
    int64_t future_us=i->first;
    NS_ASSERT_MSG(future_us>=now_us,now_us<<":"<<future_us<<":"<<alarm_map_.size());
    ns3::Time next=ns3::MicroSeconds(future_us-now_us);
    timer_=ns3::Simulator::Schedule(next,&Ns3QuicAlarmEngine::OnTimeout,this);
}
void Ns3QuicAlarmEngine::OnTimeout(){
    ns3::Time now=ns3::Simulator::Now();
    int64_t now_us=now.GetMicroSeconds();
    TimeToAlarmCBMap::iterator erase_it;
    std::vector<AlarmCB*> cbs;
    bool has_event=false;
    int64_t next_timeout_us=0;
    for(auto i=alarm_map_.begin();i!=alarm_map_.end();){
        if(i->first>now_us){
            break;
        }
        AlarmCB* cb = i->second;
        erase_it=i;
        i++;
        alarm_map_.erase(erase_it);
        all_alarms_.erase(cb);
        cbs.push_back(cb);
        has_event=true;
    }
    for(auto it=cbs.begin();it!=cbs.end();it++){
        AlarmCB *cb=(*it);
        int c=0;
        do{
            next_timeout_us=cb->OnAlarm();
            c++;
            #if (MOD_DEBUG)
            if((next_timeout_us>0)&&(next_timeout_us<=now_us)){
                NS_LOG_FUNCTION(now_us<<next_timeout_us<<c);
            }
            #endif
        }while((next_timeout_us>0)&&(next_timeout_us<=now_us));
        if(next_timeout_us>0){
            RegisterAlarm(next_timeout_us,cb);
        }
    }
    if(has_event&&visitor_){
        visitor_->PostProcessing();
    }
    UpdateTimer();
}
BaseAlarm::BaseAlarm() : engine_(NULL), registered_(false) {}

BaseAlarm::~BaseAlarm() { UnregisterIfRegistered(); }

int64_t BaseAlarm::OnAlarm() {
  registered_ = false;
  return 0;
}

void BaseAlarm::OnRegistration(const Ns3QuicAlarmEngine::AlarmRegToken& token,
                                Ns3QuicAlarmEngine* engine) {
  QUICHE_DCHECK_EQ(false, registered_);

  token_ = token;
  engine_ =engine;
  registered_ = true;
}

void BaseAlarm::OnUnregistration() { registered_ = false; }

void BaseAlarm::OnShutdown(Ns3QuicAlarmEngine* /*engine*/) {
  registered_ = false;
  engine_ = NULL;
}

// If the alarm was registered, unregister it.
void BaseAlarm::UnregisterIfRegistered() {
  if (!registered_) {
    return;
  }

  engine_->UnregisterAlarm(token_);
}

void BaseAlarm::ReregisterAlarm(int64_t timeout_time_in_us) {
  QUICHE_DCHECK(registered_);
  token_ = engine_->ReregisterAlarm(token_, timeout_time_in_us);
}
class Ns3Alarm:public QuicAlarm{
public:
    Ns3Alarm(Ns3QuicAlarmEngine* engine,QuicArenaScopedPtr<QuicAlarm::Delegate> delegate)
        :QuicAlarm(std::move(delegate)),
        engine_(engine),
        ns3_alarm_impl_(this) {
    }
protected:
    void SetImpl() override {
        QUICHE_DCHECK(deadline().IsInitialized());
        int64_t timeout_us=(deadline() - QuicTime::Zero()).ToMicroseconds();
        engine_->RegisterAlarm(timeout_us, &ns3_alarm_impl_);
    }
    
    void CancelImpl() override {
        QUICHE_DCHECK(!deadline().IsInitialized());
    ns3_alarm_impl_.UnregisterIfRegistered();
    }

  void UpdateImpl() override {
    QUICHE_DCHECK(deadline().IsInitialized());
    int64_t timeout_us = (deadline() - QuicTime::Zero()).ToMicroseconds();
    if (ns3_alarm_impl_.registered()) {
      ns3_alarm_impl_.ReregisterAlarm(timeout_us);
    } else {
      engine_->RegisterAlarm(timeout_us, &ns3_alarm_impl_);
    }
  }

private:
  class Ns3AlarmImpl : public BaseAlarm{
   public:

    explicit Ns3AlarmImpl(Ns3Alarm* alarm) : alarm_(alarm) {}
    void *alarm_addr() override {return alarm_;}
    // Use the same integer type as the base class.
    int64_t OnAlarm() override {
      BaseAlarm::OnAlarm();
      alarm_->Fire();
      // Fire will take care of registering the alarm, if needed.
      return 0;
    }

   private:
    Ns3Alarm* alarm_;
  };

  Ns3QuicAlarmEngine* engine_;
  Ns3AlarmImpl ns3_alarm_impl_;
};
Ns3AlarmFactory::Ns3AlarmFactory(Ns3QuicAlarmEngine *engine):engine_(engine){}
Ns3AlarmFactory::~Ns3AlarmFactory()=default;
QuicAlarm* Ns3AlarmFactory::CreateAlarm(QuicAlarm::Delegate* delegate){
    return new Ns3Alarm(engine_,
     QuicArenaScopedPtr<QuicAlarm::Delegate>(delegate));
}
QuicArenaScopedPtr<QuicAlarm> Ns3AlarmFactory::CreateAlarm(
                            QuicArenaScopedPtr<QuicAlarm::Delegate> delegate,
                            QuicConnectionArena* arena){
    if (arena != nullptr) {
    return arena->New<Ns3Alarm>(engine_, std::move(delegate));
    }
    return QuicArenaScopedPtr<QuicAlarm>(
        new Ns3Alarm(engine_, std::move(delegate)));
}
}
