#include "ns3-client-network-helper.h"
#include "ns3-quic-poll-server.h"
#include "ns3/assert.h"
namespace quic{
Ns3ClientNetworkHelper::Ns3ClientNetworkHelper(Ns3QuicPollServer *poller,QuicClientBase* client):
poller_(poller),client_(client){}
Ns3ClientNetworkHelper::~Ns3ClientNetworkHelper(){
  if (client_->connected()) {
    client_->session()->connection()->CloseConnection(
        QUIC_PEER_GOING_AWAY, "Client being torn down",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
  }
  CleanUpAllUDPSockets();
}
void Ns3ClientNetworkHelper::RunEventLoop(){
    NS_ASSERT_MSG(0,"run event loop should not be called");
    return;
}
bool Ns3ClientNetworkHelper::CreateUDPSocketAndBind(QuicSocketAddress server_address,
                                QuicIpAddress bind_to_address,
                                int bind_to_port){
    bool ret=poller_->CreateUDPSocketAndBind(server_address,bind_to_address,bind_to_port);
    if(ret){
        poller_->RegisterFD(this);
    }
    return ret;
}
void Ns3ClientNetworkHelper::CleanUpAllUDPSockets(){
    poller_->UnregisterFD(this);
}
QuicSocketAddress Ns3ClientNetworkHelper::GetLatestClientAddress() const{
    return poller_->GetLatestClientAddress();
}
QuicPacketWriter* Ns3ClientNetworkHelper::CreateQuicPacketWriter(){
    return poller_->CreateQuicPacketWriter();
}
void Ns3ClientNetworkHelper::ProcessPacket(const QuicSocketAddress& self_address,
                     const QuicSocketAddress& peer_address,
                     const QuicReceivedPacket& packet){
    client_->session()->ProcessUdpPacket(self_address, peer_address, packet);
}
void Ns3ClientNetworkHelper::OnRegistration(){
    
}
void Ns3ClientNetworkHelper::OnUnregistration(){
    
}
void Ns3ClientNetworkHelper::OnShutdown(){
    
}
}
