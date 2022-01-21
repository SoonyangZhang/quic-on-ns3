#pragma once
#include <string>
#include "gquiche/quic/platform/api/quic_logging.h"
#include "gquiche/quic/platform/api/quic_socket_address.h"
#include "ns3/inet-socket-address.h"
namespace ns3{
quic::QuicSocketAddress GetQuicSocketAddr(const InetSocketAddress &addr);
InetSocketAddress GetNs3SocketAddr(const quic::QuicSocketAddress &addr);
void set_quic_log_level(quiche::QuicLogLevel level);
bool set_quic_key_value(const std::string &key,const std::string &value);
bool set_quic_cert_key(const std::string& quic_cert_path);
}
