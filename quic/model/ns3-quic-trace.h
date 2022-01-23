#pragma once
#include <fstream>
#include <string>
#include <map>
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/callback.h"
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
    E_QS_ALL=E_QS_OWD,
};
void set_quic_trace_folder(const std::string &path);
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
    void Close();
    std::fstream m_owd;
    int64_t m_readBytes=0;
    Time m_firstPacketTime=Time(0);
    Time m_receiptTime=Time(0);
};
class Ns3QuicServerTraceDispatcher{
public:
    Ns3QuicServerTraceDispatcher();
    ~Ns3QuicServerTraceDispatcher();
    void Shutdown();
    void LogEnable(std::string &topo_id,uint8_t e);
    bool AddMonitorAddressPair(const Ns3QuicAddressPair &addr_pair);
    void OnOwd(const InetSocketAddress &c,const InetSocketAddress & s,int seq,int owd,int len);
private:
    std::string m_topoId;
    uint8_t m_e=0;
    int64_t m_readBytes=0;
    Time m_firstPacketTime=Time(0);
    Time m_receiptTime=Time(0);
    std::map<Ns3QuicAddressPair,Ns3QuicServerTrace*> m_traceDb;
};
}