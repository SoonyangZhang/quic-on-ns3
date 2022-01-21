#pragma once
#include <memory.h>
#include "ns3/simulator.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/callback.h"

#include "ns3/ns3-quic-server.h"
#include "ns3/ns3-quic-backend.h"
#include "ns3/ns3-packet-writer.h"
#include "ns3/ns3-quic-alarm-engine.h"
namespace ns3{
class QuicServerApp:public Application,
public quic::Ns3PacketWriter::Delegate{
public:
    QuicServerApp();
    ~QuicServerApp() override;
    void Bind(uint16_t port);
    InetSocketAddress GetLocalAddress() const;
    //from quic::Ns3PacketWriter::Delegate
    int WritePacket(const char* buffer,
                    size_t buf_len,
                    const quic::QuicIpAddress& self_address,
                    const quic::QuicSocketAddress& peer_address) override;
    void OnWriterDestroy() override{}
private:
    int Start();
    virtual void StartApplication() override;
    virtual void StopApplication() override;
    void RecvPacket(Ptr<Socket> socket);
    void SendToNetwork(Ptr<Packet> p,const InetSocketAddress& dest);
    quic::QuicClock *m_clock=nullptr;
    Ptr<Socket> m_socket;
    Ipv4Address m_bindIp;
    uint16_t m_bindPort=0;
    std::unique_ptr<quic::Ns3QuicServer> m_quicServer;
    std::unique_ptr<quic::EchoServerBackend> m_backend;
    std::unique_ptr<quic::Ns3QuicAlarmEngine> m_engine;
    bool m_running=false;
    uint64_t m_seq=1;
};
}
