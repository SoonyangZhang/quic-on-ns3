#pragma once
#include <string>
#include <memory>
#include "gquiche/quic/core/quic_session.h"
#include "ns3/ns3-quic-poll-server.h"
#include "ns3/ns3-packet-writer.h"
#include "ns3/ns3-quic-client.h"
#include "ns3/ns3-quic-backend.h"
#include "ns3/ns3-quic-alarm-engine.h"
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
namespace ns3{
class QuicClientApp:public Application,public quic::Ns3QuicPollServer,
public quic::Ns3PacketWriter::Delegate,
public quic::QuicSession::Visitor{
public:
    QuicClientApp();
    ~QuicClientApp() override;
    void Bind(uint16_t port=0);
    void set_peer(const Ipv4Address& ip_addr,uint16_t port);
    InetSocketAddress GetLocalAddress() const;
    //from Ns3QuicPollServer
    bool CreateUDPSocketAndBind(quic::QuicSocketAddress server_address,
                                quic::QuicIpAddress bind_to_address,
                                int bind_to_port) override;
    quic::QuicSocketAddress GetLatestClientAddress() const override;
    void RegisterFD(quic::Ns3PacketInCallback *cb) override;
    void UnregisterFD(quic::Ns3PacketInCallback *cb) override;
    quic::QuicPacketWriter* CreateQuicPacketWriter() override;
    //from Ns3PacketWriter::Delegate
    int WritePacket(const char* buffer,
                    size_t buf_len,
                    const quic::QuicIpAddress& self_address,
                    const quic::QuicSocketAddress& peer_address) override;
    void OnWriterDestroy() override {}
    //from Session::Visitor
    // Called when the connection is closed after the streams have been closed.
    void OnConnectionClosed(quic::QuicConnectionId server_connection_id,
                            quic::QuicErrorCode error,
                            const std::string& error_details,
                            quic::ConnectionCloseSource source) override;

    // Called when the session has become write blocked.
    void OnWriteBlocked(quic::QuicBlockedWriterInterface* blocked_writer) override;

    // Called when the session receives reset on a stream from the peer.
    void OnRstStreamReceived(const quic::QuicRstStreamFrame& frame) override;

    // Called when the session receives a STOP_SENDING for a stream from the
    // peer.
    void OnStopSendingReceived(const quic::QuicStopSendingFrame& frame) override;

    // Called when a NewConnectionId frame has been sent.
    void OnNewConnectionIdSent(
        const quic::QuicConnectionId& server_connection_id,
        const quic::QuicConnectionId& new_connection_id) override;

    // Called when a ConnectionId has been retired.
    void OnConnectionIdRetired(const quic::QuicConnectionId& server_connection_id) override;
private:
    void InitialAndConnect();
    void RecvPacket(Ptr<Socket> socket);
    void SendToNetwork(Ptr<Packet> p,const InetSocketAddress& dest);
    virtual void StartApplication() override;
    virtual void StopApplication() override;
    quic::Ns3PacketInCallback *m_packetProcessor=nullptr;
    Ptr<Socket> m_socket;
    Ipv4Address m_bindIp;
    uint16_t m_bindPort=0;
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
    std::unique_ptr<quic::Ns3QuicClient> m_quicClient;
    std::unique_ptr<quic::HelloClientBackend> m_backend;
    std::unique_ptr<quic::Ns3QuicAlarmEngine> m_engine;
    quic::QuicClock *m_clock=nullptr;
    bool m_running=false;
    uint64_t m_seq=1;
};
}
