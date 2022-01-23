#pragma once
#include <fstream>
#include <string>
#include <map>
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/ns3-quic-util.h"
namespace ns3{
//log name format: topoid_ip1:port_ip2:port_type.txt
enum QuicClientTraceEnable:uint8_t{
    E_QC_CWND=0x01,
    E_QC_IN_FLIGHT=0x02,
    E_QC_SEND_RATE=0x04,
    E_QC_ALL=E_QC_CWND|E_QC_IN_FLIGHT|E_QC_SEND_RATE,
};
enum QuicServerTraceEnabel:uint8_t{
    E_QS_OWD=0x01,
    E_QS_GOODPUT=0x02,
    E_QS_LOST=0x04,
    E_QS_ALL=E_QS_OWD|E_QS_GOODPUT|E_QS_LOST,
};
void set_quic_trace_folder(const std::string &path);
void set_quic_goodput_interval(Time interval);
class Ns3QuicClientTrace{
public:
    Ns3QuicClientTrace();
    ~Ns3QuicClientTrace();
    void LogEnable(const std::string &prefix,uint8_t e);
    void OnSendRate(float v);
    void OnCwnd(int v);
    void OnInFlight(int v);
private:
    void OpenCwndFile(const std::string& name);
    void OpenInFlightFile(const std::string& name);
    void OpenRateFile(const std::string& name);
    void Close();
    int m_uuid;
    std::fstream m_rate;
    std::fstream m_cwnd;
    std::fstream m_inFlight;
};
class Ns3QuicServerTrace{
public:
    Ns3QuicServerTrace(std::string &prefix,uint8_t e);
    ~Ns3QuicServerTrace();
    void OnOwd(int seq,int owd,int len);
private:
    void OpenOwdFile(const std::string& name);
    void OpenLostFile(const std::string& name);
    void OpenGoodputFile(const std::string& name);
    void LogLostPackets();
    void LogGoodput(bool dtor);
    void Close();
    int m_lastSeq=0;
    int m_largestSeq=0;
    int m_lostPackets=0;
    int64_t m_lastRateBytes=0;
    Time m_lastRateTime=Time(0);
    int64_t m_readBytes=0;
    Time m_firstPacketTime=Time(0);
    Time m_receiptTime=Time(0);
    std::fstream m_owd;
    std::fstream m_lost;
    std::fstream m_goodput;
};
class Ns3QuicServerTraceDispatcher{
public:
    Ns3QuicServerTraceDispatcher(DataRate rate,Time start_time);
    ~Ns3QuicServerTraceDispatcher();
    void Shutdown();
    void LogEnable(std::string &topo_id,uint8_t e);
    bool AddMonitorAddressPair(const Ns3QuicAddressPair &addr_pair);
    void OnOwd(const InetSocketAddress &c,const InetSocketAddress & s,int seq,int owd,int len);
private:
    void OpenUtilFile(const std::string& topo_id);
    DataRate m_bottleneckRate;
    Time m_startTime;
    std::string m_topoId;
    uint8_t m_e=0;
    int64_t m_readBytes=0;
    Time m_firstPacketTime=Time(0);
    Time m_receiptTime=Time(0);
    std::map<Ns3QuicAddressPair,Ns3QuicServerTrace*> m_traceDb;
    std::fstream m_util;
};
}