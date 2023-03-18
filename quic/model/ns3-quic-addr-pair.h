#pragma once
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
}
