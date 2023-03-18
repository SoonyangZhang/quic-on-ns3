#pragma once
#include <memory>
#include <memory.h>
#include "ns3/simulator.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/callback.h"
#include "ns3/ns3-quic-public.h"
namespace ns3{
class QuicServerAppImpl;
class Ns3QuicServerTraceDispatcher;
class QuicServerApp: public Application{
public:
    QuicServerApp(quic::BackendType type=quic::BackendType::HELLO_UNIDI);
    ~QuicServerApp() override;
    void Bind(uint16_t port);
    InetSocketAddress GetLocalAddress() const;
    void set_trace(Ns3QuicServerTraceDispatcher *trace);
    void SendToNetwork(const char *buffer,size_t sz,const InetSocketAddress& dest);
private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;
    void RecvPacket(Ptr<Socket> socket);
    
    std::unique_ptr<QuicServerAppImpl> m_impl;
    Ptr<Socket> m_socket;
    Ipv4Address m_bindIp;
    uint16_t m_bindPort=0;
    Ns3QuicServerTraceDispatcher *m_trace=nullptr;
    bool m_running=false;
    uint64_t m_seq=1;
};
}
