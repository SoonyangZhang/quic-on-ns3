#include <iostream>
#include <memory>
#include "gquiche/quic/core/quic_constants.h"
#include "gquiche/quic/platform/api/quic_default_proof_providers.h"
#include "gquiche/quic/platform/api/quic_logging.h"
#include "gquiche/quic/core/quic_one_block_arena.h"

#include "ns3-quic-clock.h"
#include "ns3/quic-test.h"

#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/ns3-quic-util.h"
#include "ns3-quic-addr-convert.h"
#include "ns3-quic-flags.h"
namespace quic{
void set_test_value(const std::string &v){
    std::string key("test_value");
    ns3::set_quic_key_value(key,v);
}
void print_test_value(){
    int32_t test_value=get_test_value();
    std::cout<<test_value<<std::endl;
}
void parser_flags(const char* usage,int argc, const char* const* argv){
    quic::QuicParseCommandLineFlags(usage, argc, argv);
}
void print_address(){
    {
        uint16_t port=1234;
        ns3::InetSocketAddress ns3_socket_addr("10.0.1.1",port);
        quic::QuicSocketAddress quic_socket_addr=ns3::GetQuicSocketAddr(ns3_socket_addr);
        std::cout<<quic_socket_addr.ToString()<<std::endl;
    }
    {
        uint16_t port=5678;
        quic::QuicIpAddress quic_ip_addr;
        quic_ip_addr.FromString("10.0.2.1");
        quic::QuicSocketAddress quic_socket_addr(quic_ip_addr,port);
        ns3::InetSocketAddress ns3_socket_addr=ns3::GetNs3SocketAddr(quic_socket_addr);
        std::cout<<ns3_socket_addr.GetIpv4()<<":"<<ns3_socket_addr.GetPort()<<std::endl;
    }
}
void test_quic_log(){
    std::cout<<quiche::IsLogLevelEnabled(quiche::INFO)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::WARNING)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::ERROR)<<std::endl;
    std::cout<<quiche::IsLogLevelEnabled(quiche::FATAL)<<std::endl;
    QUIC_LOG(INFO)<<"hello logger info";
    QUIC_LOG(ERROR)<<"hello logger error";
    QUIC_DLOG(FATAL) << "Unable to read key.";
    QUICHE_CHECK(0);
}
void test_proof_source(){
    std::unique_ptr<quic::ProofSource> proof_source=quic::CreateDefaultProofSource();
    if(!proof_source){
        QUIC_LOG(ERROR)<<"quic_cert path is not right"<<std::endl;
    }
}
}
