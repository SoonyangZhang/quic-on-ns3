#pragma once
#include <map>
#include <unordered_set>
#include <string>
#include "ns3/object.h"
#include "ns3/event-id.h"

#include "gquiche/quic/core/quic_alarm.h"
#include "gquiche/quic/core/quic_alarm_factory.h"
#include "gquiche/quic/core/quic_one_block_arena.h"
namespace quic{
class AlarmCallbackInterface;
class Ns3QuicAlarmEngine:public ns3::Object{
public:
    enum class Role{CLIENT,SERVER};
    typedef AlarmCallbackInterface AlarmCB;
    typedef std::multimap<int64_t, AlarmCB*> TimeToAlarmCBMap;
    typedef TimeToAlarmCBMap::iterator AlarmRegToken;
    class Visitor{
    public:
        virtual ~Visitor(){}
        virtual void PostProcessing()=0;
    };

    Ns3QuicAlarmEngine(Role role=Role::CLIENT);
    ~Ns3QuicAlarmEngine();
    void Shutdown();
    void set_visitor(Visitor *visitor){visitor_=visitor;}
    void RegisterAlarm(int64_t timeout_us,AlarmCB* ac);
    void UnregisterAlarm(const AlarmRegToken &iterator_token);
    AlarmRegToken ReregisterAlarm(AlarmRegToken &iterator_token, int64_t timeout_us);
    std::string Name();
private:
    struct AlarmCBHash {
    size_t operator()(AlarmCB* const& p) const {
        return reinterpret_cast<size_t>(p);
    }
    };
    void UpdateTimer();
    void OnTimeout();
    Role role_;
    using AlarmCBMap = std::unordered_set<AlarmCB*, AlarmCBHash>;
    AlarmCBMap all_alarms_;
    TimeToAlarmCBMap alarm_map_;
    Visitor *visitor_=nullptr;
    ns3::EventId timer_;
};
class AlarmCallbackInterface{
public:
    virtual ~AlarmCallbackInterface(){}
    // Returns:
    //   the unix time (in microseconds) at which this alarm should be signaled
    //   again, or 0 if the alarm should be removed.
    virtual int64_t OnAlarm()=0;
    virtual void OnRegistration(const Ns3QuicAlarmEngine::AlarmRegToken &token,Ns3QuicAlarmEngine *engine)=0;
    virtual void OnUnregistration()=0;
    virtual void OnShutdown(Ns3QuicAlarmEngine *engine)=0;
};
class BaseAlarm:public AlarmCallbackInterface{
public:
    BaseAlarm();
    ~BaseAlarm() override;
    // Marks the alarm as unregistered and returns 0.  The return value may be
    // safely ignored by subclasses.
    int64_t OnAlarm() override;
    
    // Marks the alarm as registered, and stores the token.
    void OnRegistration(const Ns3QuicAlarmEngine::AlarmRegToken& token,
                        Ns3QuicAlarmEngine* engine) override;
    
    // Marks the alarm as unregistered.
    void OnUnregistration() override;
    
    // Marks the alarm as unregistered.
    void OnShutdown(Ns3QuicAlarmEngine* engine) override;
    
    // If the alarm was registered, unregister it.
    void UnregisterIfRegistered();
    
    // Reregisters the alarm at specified time.
    void ReregisterAlarm(int64_t timeout_time_in_us);
    
    bool registered() const { return registered_; }
    
    const Ns3QuicAlarmEngine* eps() const { return engine_; }
    
private:
    Ns3QuicAlarmEngine::AlarmRegToken token_;
    Ns3QuicAlarmEngine* engine_;
    bool registered_;
};
class Ns3AlarmFactory:public  QuicAlarmFactory{
public:
    Ns3AlarmFactory(Ns3QuicAlarmEngine *engine);
    ~Ns3AlarmFactory();
    // QuicAlarmFactory interface.
    QuicAlarm* CreateAlarm(QuicAlarm::Delegate* delegate) override;
    QuicArenaScopedPtr<QuicAlarm> CreateAlarm(
        QuicArenaScopedPtr<QuicAlarm::Delegate> delegate,
        QuicConnectionArena* arena) override;
private:
    Ns3QuicAlarmEngine *engine_=nullptr;
};
}
