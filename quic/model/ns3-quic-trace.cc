#include <unistd.h>
#include <dirent.h>

#include "ns3/simulator.h"
#include "ns3-quic-trace.h"
#include "ns3-quic-util.h"
namespace ns3{
namespace{
    std::string Ns3QuicTracePath;
    int g_uuid=1;
    Time g_goodputInterval=MilliSeconds(1000);
}
static inline int uuid_allocator(){
    return g_uuid++;
}
void set_quic_trace_folder(const std::string &path){
    int len=path.size();
    if(len&&Ns3QuicTracePath[len-1]!='/'){
            Ns3QuicTracePath=path+'/';
    }else{
        Ns3QuicTracePath=path;
    }
}
void set_quic_goodput_interval(Time interval){
    g_goodputInterval=interval;
}
Ns3QuicClientTrace::Ns3QuicClientTrace(){
    m_uuid=uuid_allocator();
}
Ns3QuicClientTrace::~Ns3QuicClientTrace(){
    Close();
}

void Ns3QuicClientTrace::LogEnable(const std::string &prefix,QuicClientTraceType e){
    char buf[FILENAME_MAX];
    memset(buf,0,FILENAME_MAX);
    std::string name;
    if(0==Ns3QuicTracePath.size()){
        name=std::string (getcwd(buf, FILENAME_MAX))+
                "/traces/"+prefix;
    }else{
        name=std::string(Ns3QuicTracePath)+prefix;
    }
    if(e&E_QC_CWND){
        OpenCwndFile(name);
    }
    if(e&E_QC_IN_FLIGHT){
        OpenInFlightFile(name);
    }
    if(e&E_QC_SEND_RATE){
        OpenRateFile(name);
    }
}
void Ns3QuicClientTrace::OnSendRate(float v){
    if(m_rate.is_open()){
        float now=Simulator::Now().GetSeconds();
        m_rate<<now<<" "<<v<<std::endl;
    }
}
void Ns3QuicClientTrace::OnCwnd(int v){
    if(m_cwnd.is_open()){
        float now=Simulator::Now().GetSeconds();
        m_cwnd<<now<<" "<<v<<std::endl;
    }
}
void Ns3QuicClientTrace::OnInFlight(int v){
    if(m_inFlight.is_open()){
        float now=Simulator::Now().GetSeconds();
        m_inFlight<<now<<" "<<v<<std::endl;
    }
}
void Ns3QuicClientTrace::OpenCwndFile(const std::string& name){
    std::string path_name=name+"_cwnd.txt";
    m_cwnd.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicClientTrace::OpenInFlightFile(const std::string& name){
    std::string path_name=name+"_inflight.txt";
    m_inFlight.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicClientTrace::OpenRateFile(const std::string& name){
    std::string path_name=name+"_sendrate.txt";
    m_rate.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicClientTrace::Close(){
    if(m_rate.is_open()){
        m_rate.close();
    }
    if(m_cwnd.is_open()){
        m_cwnd.close();
    }
    if(m_inFlight.is_open()){
        m_inFlight.close();
    }
}

Ns3QuicServerTrace::Ns3QuicServerTrace(std::string &prefix,QuicServerTraceType e){
    char buf[FILENAME_MAX];
    memset(buf,0,FILENAME_MAX);
    std::string name;
    if(0==Ns3QuicTracePath.size()){
        name=std::string (getcwd(buf, FILENAME_MAX))+
                "/traces/"+prefix;
    }else{
        name=std::string(Ns3QuicTracePath)+prefix;
    }
    if(e&E_QS_OWD){
        OpenOwdFile(name);
    }
    if(e&E_QS_LOST){
        OpenLostFile(name);
    }
    if(e&E_QS_GOODPUT){
        OpenGoodputFile(name);
    }
}
Ns3QuicServerTrace::~Ns3QuicServerTrace(){
    LogGoodput(true);
    LogLostPackets();
    Close();
}
void Ns3QuicServerTrace::OnOwd(int seq,int owd,int len){
    if(m_owd.is_open()||m_goodput.is_open()){
        Time now=Simulator::Now();
        if(Time(0)==m_firstPacketTime){
            m_firstPacketTime=now;
            m_lastRateTime=now;
        }
        m_readBytes+=len;
        if(m_owd.is_open()){
            m_owd<<now.GetSeconds()<<"\t"<<seq<<"\t"<<owd<<"\t"<<len<<std::endl;
            m_receiptTime=now;
        }
        LogGoodput(false);
    }

    if(m_lost.is_open()){
        if(seq>m_lastSeq-1){
            int lost=seq-m_lastSeq-1;
            m_lostPackets+=lost;
        }
        if(seq>m_lastSeq){
            m_lastSeq=seq;
        }
        if(seq>m_largestSeq){
            m_largestSeq=seq;
        }
    }
}
void Ns3QuicServerTrace::OpenOwdFile(const std::string& name){
    std::string path_name=name+"_owd.txt";
    m_owd.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicServerTrace::OpenLostFile(const std::string& name){
    std::string path_name=name+"_lost.txt";
    m_lost.open(path_name.c_str(), std::fstream::out);
}
void  Ns3QuicServerTrace::OpenGoodputFile(const std::string& name){
    std::string path_name=name+"_goodput.txt";
    m_goodput.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicServerTrace::LogLostPackets(){
    if(m_lost.is_open()){
        float ratio=0.0;
        if(m_largestSeq>0){
            ratio=1.0*m_lostPackets*100/m_largestSeq;
        }
        m_lost<<m_lostPackets<<"\t"<<m_largestSeq<<"\t"<<ratio<<std::endl;
    }
}
void Ns3QuicServerTrace::LogGoodput(bool dtor){
    if(m_goodput.is_open()){
        if(dtor){
            if(m_receiptTime>m_lastRateTime){
                Time delta=m_receiptTime-m_lastRateTime;
                float kbps=1.0*(m_readBytes-m_lastRateBytes)*8/delta.GetMilliSeconds();
                m_goodput<<m_receiptTime.GetSeconds()<<" "<<kbps<<std::endl;
            }
        }else{
            if(m_receiptTime>=m_lastRateTime+g_goodputInterval){
                Time delta=m_receiptTime-m_lastRateTime;
                float kbps=1.0*(m_readBytes-m_lastRateBytes)*8/delta.GetMilliSeconds();
                m_goodput<<m_receiptTime.GetSeconds()<<" "<<kbps<<std::endl;
                m_lastRateBytes=m_readBytes;
                m_lastRateTime=m_receiptTime;
            }
        }
    }
}
void Ns3QuicServerTrace::Close(){
    if(m_owd.is_open()){
        m_owd.close();
    }
    if(m_lost.is_open()){
        m_lost.close();
    }
    if(m_goodput.is_open()){
        m_goodput.close();
    }
}
Ns3QuicServerTraceDispatcher::Ns3QuicServerTraceDispatcher(){}
Ns3QuicServerTraceDispatcher::~Ns3QuicServerTraceDispatcher(){
    Shutdown();
}
void Ns3QuicServerTraceDispatcher::Shutdown(){
    while(!m_traceDb.empty()){
        auto it=m_traceDb.begin();
        Ns3QuicServerTrace *trace=it->second;
        m_traceDb.erase(it);
        delete trace;
    }
}
void Ns3QuicServerTraceDispatcher::LogEnable(std::string &topo_id,QuicServerTraceType e){
    m_topoId=topo_id;
    m_e=e;
    if(E_QS_UTIL&&e){
        OpenUtilFile(topo_id);
    }
}
bool Ns3QuicServerTraceDispatcher::AddMonitorAddressPair(const Ns3QuicAddressPair &addr_pair){
    bool success=false;
    auto it=m_traceDb.find(addr_pair);
    if(it==m_traceDb.end()){
        std::string prefix=m_topoId+"_"+Ns3QuicAddressPair2String(addr_pair);
        Ns3QuicServerTrace *trace=new Ns3QuicServerTrace(prefix,m_e);
        m_traceDb.insert(std::make_pair(addr_pair,trace));
        success=true;
    }
    return success;
}
void Ns3QuicServerTraceDispatcher::OnOwd(const InetSocketAddress &c,const InetSocketAddress & s,int seq,int owd,int len){
    Time now=Simulator::Now();
    m_readBytes+=len;
    if(Time(0)==m_firstPacketTime){
        m_firstPacketTime=now;
    }
    m_receiptTime=now;
    Ns3QuicAddressPair addr_pair(c,s);
    auto it=m_traceDb.find(addr_pair);
    if(it!=m_traceDb.end()){
        it->second->OnOwd(seq,owd,len);
    }
}

void Ns3QuicServerTraceDispatcher::CalculateUtil(int64_t channel_bit){
    if (f_util.is_open() && channel_bit > 0) {
        float util=1.0*m_readBytes*8*100/(channel_bit);
        f_util<<util<<std::endl;
        f_util.close();
    }
}
void Ns3QuicServerTraceDispatcher::OpenUtilFile(const std::string& topo_id){
    char buf[FILENAME_MAX];
    memset(buf,0,FILENAME_MAX);
    std::string name;
    if(0==Ns3QuicTracePath.size()){
        name=std::string (getcwd(buf, FILENAME_MAX))+
                "/traces/"+topo_id+"_util.txt";
    }else{
        name=std::string(Ns3QuicTracePath)+topo_id+"_util.txt";
    }
    f_util.open(name.c_str(), std::fstream::out);
}

}
