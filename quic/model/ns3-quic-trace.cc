#include <unistd.h>
#include <dirent.h>

#include "ns3/simulator.h"
#include "ns3-quic-trace.h"
#include "ns3-quic-util.h"
namespace ns3{
std::string Ns3QuicTracePath;
int uuid=1;
static inline int uuid_allocator(){
    return uuid++;
}
void set_quic_trace_folder(const std::string &path){
    int len=path.size();
    if(len&&Ns3QuicTracePath[len-1]!='/'){
            Ns3QuicTracePath=path+'/';
    }else{
        Ns3QuicTracePath=path;
    }
}
Ns3QuicClientTrace::Ns3QuicClientTrace(){
    m_uuid=uuid_allocator();
}
Ns3QuicClientTrace::~Ns3QuicClientTrace(){
    Close();
}

void Ns3QuicClientTrace::LogEnable(const std::string &prefix,uint8_t e){
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

Ns3QuicServerTrace::Ns3QuicServerTrace(std::string &prefix,uint8_t e){
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
}
Ns3QuicServerTrace::~Ns3QuicServerTrace(){
    Close();
}
void Ns3QuicServerTrace::OnOwd(int seq,int owd,int len){
    if(m_owd.is_open()){
        Time now=Simulator::Now();
        m_readBytes+=len;
        if(Time(0)==m_firstPacketTime){
            m_firstPacketTime=now;
        }
        m_owd<<now.GetSeconds()<<"\t"<<seq<<"\t"<<owd<<"\t"<<len<<std::endl;
        m_receiptTime=now;
    }

}
void Ns3QuicServerTrace::OpenOwdFile(const std::string& name){
    std::string path_name=name+"_owd.txt";
    m_owd.open(path_name.c_str(), std::fstream::out);
}
void Ns3QuicServerTrace::Close(){
    if(m_owd.is_open()){
        m_owd.close();
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
void Ns3QuicServerTraceDispatcher::LogEnable(std::string &topo_id,uint8_t e){
    m_topoId=topo_id;
    m_e=e;
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
    Ns3QuicAddressPair addr_pair(c,s);
    auto it=m_traceDb.find(addr_pair);
    if(it!=m_traceDb.end()){
        it->second->OnOwd(seq,owd,len);
    }
    m_receiptTime=now;
}

}
