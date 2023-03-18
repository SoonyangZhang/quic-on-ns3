#include "gquiche/quic/tools/fake_proof_verifier.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3-quic-util.h"
#include "ns3-quic-tag.h"

#include "ns3-quic-client-app.h"
#include "ns3-quic-clock.h"

#include <iostream>
#include "ns3-quic-flags.h"

namespace ns3{
NS_LOG_COMPONENT_DEFINE("QuicClientApp");
QuicClientApp::QuicClientApp(quic::BackendType backend_type,
quic::CongestionControlType cc_type):
m_ccType(cc_type){
    m_clock=quic::Ns3QuicClock::Instance();
    m_engine.reset(new quic::Ns3QuicAlarmEngine(quic::Perspective::IS_CLIENT));
    if(quic::BackendType::HELLO_UNIDI==backend_type){
        m_backend.reset(new quic::HelloClientBackend(m_engine.get(),false));
    }else if(quic::BackendType::HELLO_BIDI==backend_type){
        m_backend.reset(new quic::HelloClientBackend(m_engine.get(),true));
    }else{
        m_backend.reset(new quic::BandwidthClientBackend(m_engine.get()));
    }
}
QuicClientApp::~QuicClientApp(){
    StopApplication();
    if(m_socket){
        m_socket->Close();
        m_socket=nullptr;
    }
}

void QuicClientApp::Bind(uint16_t port){
    if (NULL==m_socket){
        m_socket = Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (),port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
        Address addr;
        m_socket->GetSockName(addr);
        InetSocketAddress self_sock_addr=InetSocketAddress::ConvertFrom(addr);
        m_bindPort=self_sock_addr.GetPort();
        m_socket->SetRecvCallback (MakeCallback(&QuicClientApp::RecvPacket,this));
    }
}
void QuicClientApp::set_peer(const Ipv4Address& ip_addr,uint16_t port){
    m_peerIp=ip_addr;
    m_peerPort=port;
}
InetSocketAddress QuicClientApp::GetLocalAddress() const{
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal();
    return InetSocketAddress{local_ip,m_bindPort}; 
}
bool QuicClientApp::CreateUDPSocketAndBind(quic::QuicSocketAddress server_address,
                            quic::QuicIpAddress bind_to_address,
                            int bind_to_port){
    return true;
}
quic::QuicSocketAddress QuicClientApp::GetLatestClientAddress() const{
    InetSocketAddress ns3_socket_addr=GetLocalAddress();
    return GetQuicSocketAddr(ns3_socket_addr);
}
void QuicClientApp::RegisterFD(quic::Ns3PacketInCallback *cb){
    m_packetProcessor=cb;
    m_packetProcessor->OnRegistration();
    
}
void QuicClientApp::UnregisterFD(quic::Ns3PacketInCallback *cb){
    if(m_packetProcessor==cb){
        cb->OnUnregistration();
        m_packetProcessor=nullptr;
    }
    
}
quic::QuicPacketWriter* QuicClientApp::CreateQuicPacketWriter(){
    quic::QuicPacketWriter *writer=new quic::Ns3PacketWriter(this);
    return writer;
}
int QuicClientApp::WritePacket(const char* buffer,
size_t buf_len,
const quic::QuicIpAddress& self_address,
const quic::QuicSocketAddress& peer_address){
    SendToNetwork(buffer,buf_len);
    return buf_len;
}

void QuicClientApp::OnConnectionClosed(quic::QuicConnectionId server_connection_id,
                        quic::QuicErrorCode error,
                        const std::string& error_details,
                        quic::ConnectionCloseSource source){
    NS_LOG_ERROR("OnConnectionClosed "<<error_details);
}
void QuicClientApp::OnWriteBlocked(quic::QuicBlockedWriterInterface* blocked_writer){
    
}
void QuicClientApp::OnRstStreamReceived(const quic::QuicRstStreamFrame& frame){
    
}
void QuicClientApp::OnStopSendingReceived(const quic::QuicStopSendingFrame& frame){
    
}
void  QuicClientApp::OnNewConnectionIdSent(
        const quic::QuicConnectionId& server_connection_id,
        const quic::QuicConnectionId& new_connection_id){
            
}
void QuicClientApp::OnConnectionIdRetired(const quic::QuicConnectionId& server_connection_id){
    
}
void QuicClientApp::SetRateTraceFuc(TraceOneFloatValue cb){
    m_traceRate=cb;
}
void QuicClientApp::SetCwndTraceFun(TraceOneIntValue cb){
    m_traceCwnd=cb;
}
void QuicClientApp::SetInFlightTraceFun(TraceOneIntValue cb){
    m_traceInFlight=cb;
}
void QuicClientApp::InitialAndConnect(){
    if(m_quicClient){return ;}

    quic::ParsedQuicVersionVector versions = quic::CurrentSupportedVersions();

    std::string quic_version_string =quic::get_quic_version();
    if (quic::get_quic_ietf_draft()) {
        quic::QuicVersionInitializeSupportForIetfDraft();
        versions = {};
        for (const quic::ParsedQuicVersion& version : quic::AllSupportedVersions()) {
            if (version.HasIetfQuicFrames() &&
                version.handshake_protocol == quic::PROTOCOL_TLS1_3) {
            versions.push_back(version);
            }
        }
        quic::QuicEnableVersion(versions[0]);
    
    } else if (!quic_version_string.empty()) {
        quic::ParsedQuicVersion parsed_quic_version =
            quic::ParseQuicVersionString(quic_version_string);
        if (parsed_quic_version.transport_version ==
            quic::QUIC_VERSION_UNSUPPORTED) {
            return ;
        }
        versions = {parsed_quic_version};
        quic::QuicEnableVersion(parsed_quic_version);
    }
    
    if (quic::get_force_version_negotiation()) {
        versions.insert(versions.begin(),
                        quic::QuicVersionReservedForNegotiation());
    }
    
    std::unique_ptr<quic::ProofVerifier> proof_verifier=std::make_unique<quic::FakeProofVerifier>();
    InetSocketAddress ns3_peer_sock_addr(m_peerIp,m_peerPort); 
    InetSocketAddress ns3_self_sock_addr(m_bindIp,m_bindPort);
    quic::QuicSocketAddress quic_peer_sock_addr=GetQuicSocketAddr(ns3_peer_sock_addr);
    quic::QuicServerId server_id(quic_peer_sock_addr.host().ToString(),
                                 quic_peer_sock_addr.port());
    m_quicClient.reset(new quic::Ns3QuicClient(this,quic_peer_sock_addr,server_id,
                       versions,std::move(proof_verifier),m_engine.get(),m_backend.get(),this,m_ccType));
    
    quic::QuicSocketAddress quic_self_sock_addr=GetQuicSocketAddr(ns3_self_sock_addr);
    m_quicClient->set_bind_to_address(quic_self_sock_addr.host());
    m_quicClient->set_initial_max_packet_length(quic::kDefaultMaxPacketSize);
    if(!m_quicClient->Initialize()){
        NS_LOG_ERROR("failed to initial client");
        return ;
    }
    m_quicClient->AsynConnect();
}
void QuicClientApp::RecvPacket(Ptr<Socket> socket){
    if (!m_running) {return ;}
    quic::QuicTime now=m_clock->Now();
    InetSocketAddress ns3_self_sock_addr(m_bindIp,m_bindPort);
    quic::QuicSocketAddress quic_self_sock_addr=GetQuicSocketAddr(ns3_self_sock_addr);
    
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    InetSocketAddress ns3_peer_sock_addr=InetSocketAddress::ConvertFrom(remoteAddr);
    quic::QuicSocketAddress quic_peer_sock_addr=GetQuicSocketAddr(ns3_peer_sock_addr);
    NS_ASSERT((ns3_peer_sock_addr.GetIpv4()==m_peerIp)&&(ns3_peer_sock_addr.GetPort()==m_peerPort));
    uint32_t length=packet->GetSize();
    //NS_LOG_INFO(this<<" QuicClientApp::recv "<<ns3_peer_sock_addr.GetIpv4()<<" "<<length);
    Ns3QuicTag tag;
    packet->RemovePacketTag(tag);
    char buffer[2000]={'\0'};
    packet->CopyData((uint8_t*)buffer,length);
    quic::QuicReceivedPacket quic_packet(buffer,(size_t)length,now);
    if(m_packetProcessor){
        m_packetProcessor->ProcessPacket(quic_self_sock_addr,quic_peer_sock_addr,quic_packet);
    }
}
void QuicClientApp::SendToNetwork(const char *buffer,size_t sz){
    if(!m_socket) {return ;}
    Time now=Simulator::Now();
    int cwnd=0;
    int in_flight=0;
    quic::QuicBandwidth bandwidth=quic::QuicBandwidth::Zero();
    if (m_quicClient) {
        if (!m_traceCwnd.IsNull()) {
            m_quicClient->GetCongestionWindowPackets(cwnd);
            if (m_cwnd != cwnd) {
                m_traceCwnd(cwnd);
                m_cwnd=cwnd;
            }
        }
        if (!m_traceInFlight.IsNull()) {
            m_quicClient->GetInFlightPackets(in_flight);
            if (m_inFlight !=  in_flight) {
                m_traceInFlight(in_flight);
                m_inFlight=in_flight;
            }
        }
        if (!m_traceRate.IsNull()) {
            m_quicClient->GetBandwidth(bandwidth);
            int64_t bps=bandwidth.ToBitsPerSecond();
            if (m_rate != bps) {
                float v=1.0*bps/1000;
                m_traceRate(v);
                m_rate=bps;
            }
        }
    }
    InetSocketAddress remote(m_peerIp,m_peerPort);
    Ptr<Packet> packet=Create<Packet>((const uint8_t*)buffer,sz);
    //NS_LOG_INFO(this<<" QuicClientApp::send "<<remote.GetIpv4()<<" "<<remote.GetPort()<<" "<<now.GetSeconds());
    uint64_t send_time=now.GetMilliSeconds();
    Ns3QuicTag tag(m_seq,send_time);
    packet->AddPacketTag(tag); 
    m_seq++;
    m_socket->SendTo(packet,0,remote);
}
void QuicClientApp::StartApplication(){
    m_running=true;
    m_bindIp=GetLocalAddress().GetIpv4();
    InitialAndConnect();
}
void QuicClientApp::StopApplication(){
    if(m_quicClient){
        m_quicClient->Disconnect();
    }
    m_quicClient=nullptr;
    m_backend=nullptr;
    m_engine=nullptr;
    m_running=false;
}
}
