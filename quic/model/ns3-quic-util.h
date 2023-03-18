#pragma once
#include <string>
#include "ns3/ns3-quic-addr-pair.h"
namespace ns3{
void set_quic_log_error_level();
bool set_quic_key_value(const std::string &key,const std::string &value);
bool set_quic_cert_key(const std::string& quic_cert_path);
std::string GetNs3Ipv4Address2String(const Ipv4Address& ip);
std::string GetIp2String(uint32_t ip_host_order);
std::string Ns3QuicAddressPair2String(const Ns3QuicAddressPair& addr_pair);
bool MakePath(const std::string& path);
int WallTimeMillis();
}
