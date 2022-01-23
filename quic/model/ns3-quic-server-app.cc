#include "gquiche/quic/platform/api/quic_default_proof_providers.h"
#include "gquiche/quic/platform/api/quic_logging.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3-quic-clock.h"
#include "ns3-quic-tag.h"
#include "ns3-quic-flags.h"
#include "ns3-quic-util.h"
#include "ns3-quic-server-app.h"
#include <iostream>
namespace ns3{
NS_LOG_COMPONENT_DEFINE("QuicServerApp");
QuicServerApp::QuicServerApp(quic::BackendType type){
    m_clock=quic::Ns3QuicClock::Instance();
    m_engine.reset(new quic::Ns3QuicAlarmEngine(quic::Perspective::IS_SERVER));
    if(quic::BackendType::HELLO_UNIDI==type||quic::BackendType::HELLO_BIDI==type){
        m_backend.reset(new quic::HelloServerBackend(m_engine.get()));
    }else{
        m_backend.reset(new quic::BandwidthServerBackend(m_engine.get()));
    }
}
QuicServerApp::~QuicServerApp(){
    StopApplication();
    if(m_socket){
        m_socket->Close();
        m_socket=nullptr;
    }
}

void QuicServerApp::Bind(uint16_t port){
    if (NULL==m_socket){
        m_socket = Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (),port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
        Address addr;
        m_socket->GetSockName(addr);
        InetSocketAddress self_sock_addr=InetSocketAddress::ConvertFrom(addr);
        m_bindPort=self_sock_addr.GetPort();
        m_socket->SetRecvCallback (MakeCallback(&QuicServerApp::RecvPacket,this));
    }
}
InetSocketAddress QuicServerApp::GetLocalAddress() const{
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal();
    return InetSocketAddress{local_ip,m_bindPort}; 
}
int QuicServerApp::WritePacket(const char* buffer,
size_t buf_len,
const quic::QuicIpAddress& self_address,
const quic::QuicSocketAddress& peer_address){
    Ptr<Packet> packet=Create<Packet>((const uint8_t*)buffer,buf_len);
    InetSocketAddress ns3_socket_addr=GetNs3SocketAddr(peer_address);
    SendToNetwork(packet,ns3_socket_addr);
    return buf_len;
}
int QuicServerApp::Start(){
    quic::ParsedQuicVersionVector supported_versions;
    if (quic::get_quic_ietf_draft()) {
        quic::QuicVersionInitializeSupportForIetfDraft();
        for (const quic::ParsedQuicVersion& version : quic::AllSupportedVersions()) {
            // Add all versions that supports IETF QUIC.
            if (version.HasIetfQuicFrames() &&
                version.handshake_protocol == quic::PROTOCOL_TLS1_3) {
                supported_versions.push_back(version);
            }
        }
    } else {
        supported_versions = quic::AllSupportedVersions();
    }
    
    std::string versions_string = quic::get_quic_version();;
    if (!versions_string.empty()) {
        supported_versions = quic::ParseQuicVersionVectorString(versions_string);
    }
    if (supported_versions.empty()) {
        return 1;
    }
    for (const auto& version : supported_versions) {
        quic::QuicEnableVersion(version);
    }
    auto proof_source = quic::CreateDefaultProofSource();
    QUICHE_CHECK(proof_source!=nullptr);
    quic::Ns3QuicAlarmEngine *engine=m_backend->get_engine();
    m_quicServer.reset(new quic::Ns3QuicServer(std::move(proof_source),
                                               engine,m_backend.get()));
    quic::QuicPacketWriter *writer=new quic::Ns3PacketWriter(this);
    if(!m_quicServer->ConfigureDispatcher(writer)){
        return 1;
    }
    return 0;
}
void QuicServerApp::StartApplication(){
    m_running=true;
    m_bindIp=GetLocalAddress().GetIpv4();
    Start();
}
void QuicServerApp::StopApplication(){
    if(m_quicServer){
        //NS_LOG_INFO("QuicServerApp::StopApplication");
        m_quicServer->Shutdown();
    }
    if(m_trace){
        m_trace->Shutdown();
        m_trace=nullptr;
    }
    m_quicServer=nullptr;
    m_backend=nullptr;
    m_engine=nullptr; 
    m_running=false;
}
void QuicServerApp::RecvPacket(Ptr<Socket> socket){
    if(!m_running){return ;}
    quic::QuicTime now=m_clock->Now();
    InetSocketAddress ns3_self_sock_addr(m_bindIp,m_bindPort);
    quic::QuicSocketAddress quic_self_sock_addr=GetQuicSocketAddr(ns3_self_sock_addr);
    
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    InetSocketAddress ns3_peer_sock_addr=InetSocketAddress::ConvertFrom(remoteAddr);
    quic::QuicSocketAddress quic_peer_sock_addr=GetQuicSocketAddr(ns3_peer_sock_addr);
    uint32_t length=packet->GetSize();
    //NS_LOG_INFO(this<<" QuicServerApp::recv "<<ns3_peer_sock_addr.GetIpv4()<<" "<<length);
    Ns3QuicTag tag;
    packet->RemovePacketTag(tag);
    uint8_t buffer[2000]={'\0'};
    packet->CopyData(buffer,length);
    quic::QuicReceivedPacket quic_packet((char*)buffer,(size_t)length,now);
    if(m_quicServer){
        m_quicServer->ProcessPacket(quic_self_sock_addr,quic_peer_sock_addr,quic_packet);
    }
    if(m_trace){
        int seq=tag.GetSequence();
        int64_t now_ms=(now-quic::QuicTime::Zero()).ToMilliseconds();
        int64_t sent_time=tag.GetTime();
        int owd=0;
        if(now_ms>=sent_time){
            owd=now_ms-sent_time;
        }
        m_trace->OnOwd(ns3_peer_sock_addr,ns3_self_sock_addr,seq,owd,length);
    }
}
void QuicServerApp::SendToNetwork(Ptr<Packet> p,const InetSocketAddress& dest){
    Time now=Simulator::Now();
    //NS_LOG_INFO(this<<" QuicServerApp::send "<<dest.GetIpv4()<<" "<<dest.GetPort()<<" "<<now.GetSeconds());
    if(!m_socket) {return ;}
    uint64_t send_time=now.GetMilliSeconds();
    Ns3QuicTag tag(m_seq,send_time);
    p->AddPacketTag(tag); 
    m_seq++;
    m_socket->SendTo(p,0,dest);
}
}
