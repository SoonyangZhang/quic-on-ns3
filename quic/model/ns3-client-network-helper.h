#pragma once
#include "gquiche/quic/core/quic_packets.h"
#include "gquiche/quic/platform/api/quic_socket_address.h"
#include "gquiche/quic/tools/quic_client_base.h"
#include "gquiche/quic/core/quic_packet_writer.h"
#include "ns3-quic-poll-server.h"
namespace quic{
class Ns3QuicPollServer;
class Ns3ClientNetworkHelper:public QuicClientBase::NetworkHelper,public Ns3PacketInCallback{
public:
    Ns3ClientNetworkHelper(Ns3QuicPollServer *poller,QuicClientBase* client);
    ~Ns3ClientNetworkHelper() override;
    //from QuicClientBase::NetworkHelper
    void RunEventLoop() override;
    bool CreateUDPSocketAndBind(QuicSocketAddress server_address,
                                QuicIpAddress bind_to_address,
                                int bind_to_port) override;
    void CleanUpAllUDPSockets() override;
    QuicSocketAddress GetLatestClientAddress() const override;
    QuicPacketWriter* CreateQuicPacketWriter() override;
    //from Ns3PacketInCallback
    void ProcessPacket(const QuicSocketAddress& self_address,
                     const QuicSocketAddress& peer_address,
                     const QuicReceivedPacket& packet) override;
    void OnRegistration() override;
    void OnUnregistration() override;
    void OnShutdown() override;
private:
    Ns3QuicPollServer *poller_=nullptr;
    QuicClientBase *client_=nullptr;
};
}
