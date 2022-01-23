#pragma once
#include <string>
#include "gquiche/quic/platform/api/quic_logging.h"
#include "gquiche/quic/platform/api/quic_socket_address.h"
#include "ns3/inet-socket-address.h"
namespace ns3{
struct Ns3QuicAddressPair{
    Ns3QuicAddressPair(const InetSocketAddress &c,const InetSocketAddress & s):
    client_ip(c.GetIpv4().Get()),
    server_ip(s.GetIpv4().Get()),
    client_port(c.GetPort()),
    server_port(s.GetPort()){}
    uint32_t client_ip;
    uint32_t server_ip;
    uint16_t client_port;
    uint16_t server_port;
    bool operator < (const Ns3QuicAddressPair &o) const
    {
        return client_ip < o.client_ip||server_ip<o.server_ip||
        client_port<o.client_port||server_port<o.server_port;
    }
};
quic::QuicSocketAddress GetQuicSocketAddr(const InetSocketAddress &addr);
InetSocketAddress GetNs3SocketAddr(const quic::QuicSocketAddress &addr);
void set_quic_log_level(quiche::QuicLogLevel level);
bool set_quic_key_value(const std::string &key,const std::string &value);
bool set_quic_cert_key(const std::string& quic_cert_path);
std::string GetNs3Ipv4Address2String(const Ipv4Address& ip);
std::string GetIp2String(uint32_t ip_host_order);
std::string Ns3QuicAddressPair2String(const Ns3QuicAddressPair& addr_pair);
bool MakePath(const std::string& path);
int WallTimeMillis();
}
