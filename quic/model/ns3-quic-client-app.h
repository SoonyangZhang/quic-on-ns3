#pragma once
#include <string>
#include <memory>
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ns3-quic-public.h"
namespace ns3{
class QuicClientAppImpl;
class QuicClientApp:public Application{
public:
    QuicClientApp(quic::BackendType backend_type,const char *cc_name);
    ~QuicClientApp() override;
    void Bind(uint16_t port=0);
    void set_peer(const Ipv4Address& ip_addr,uint16_t port);
    InetSocketAddress GetLocalAddress() const;
    void RecvPacket(Ptr<Socket> socket);
    void SendToNetwork(const char *buffer,size_t sz);

    typedef Callback<void,float> TraceOneFloatValue;
    typedef Callback<void,int> TraceOneIntValue;
    void SetRateTraceFuc(TraceOneFloatValue cb);
    void SetCwndTraceFun(TraceOneIntValue cb);
    void SetInFlightTraceFun(TraceOneIntValue cb);

private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;
    Ptr<Socket> m_socket;
    Ipv4Address m_bindIp;
    uint16_t m_bindPort=0;
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
    bool m_running=false;
    uint64_t m_seq=1;
    std::unique_ptr<QuicClientAppImpl> m_impl;
    TraceOneFloatValue m_traceRate;
    TraceOneIntValue m_traceCwnd;
    TraceOneIntValue m_traceInFlight;
    int64_t m_rate=0;
    int m_cwnd=0;
    int m_inFlight=0;
};
}
