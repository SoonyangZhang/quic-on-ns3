#include <memory.h>
#include <utility>
#include <unistd.h>// access
#include "gquiche/quic/platform/api/quic_flags.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "gquiche/quic/platform/api/quic_logging.h"
#include "ns3-quic-util.h"
namespace ns3{
namespace{
    static bool quic_log_level_setted=false;
}
quic::QuicSocketAddress GetQuicSocketAddr(const InetSocketAddress &addr){
    Ipv4Address ns3_ip_addr=addr.GetIpv4();
    uint16_t port=addr.GetPort();
    uint32_t ip_host_order=ns3_ip_addr.Get();
    uint32_t ip_net_order=htonl(ip_host_order);
    in_addr addr_in;
    memcpy(&addr_in,&ip_net_order,sizeof(uint32_t));
    quic::QuicIpAddress quic_ip_addr(addr_in);
    return quic::QuicSocketAddress(quic_ip_addr,port);
}
InetSocketAddress GetNs3SocketAddr(const quic::QuicSocketAddress &addr){
    quic::QuicIpAddress quic_ip_addr=addr.host();
    uint16_t port=addr.port();
    in_addr addr_in=quic_ip_addr.GetIPv4();
    uint32_t ip_net_order=0;
    memcpy(&ip_net_order,&addr_in,sizeof(uint32_t));
    uint32_t ip_host_order=ntohl(ip_net_order);
    Ipv4Address ns3_ip_addr(ip_host_order);
    return InetSocketAddress{ns3_ip_addr,port};
}
void set_quic_log_level(quiche::QuicLogLevel level){
    if(!quic_log_level_setted){
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        quiche::GetLogger().sinks().push_back(sink);
        quiche::GetLogger().set_level(quiche::ERROR);
        quic_log_level_setted=true;
    }
}
bool set_quic_key_value(const std::string &key,const std::string &value){
    bool success=false;
    auto flag_ptr = quiche::FlagRegistry::GetInstance().FindFlag(key);
    if(flag_ptr){
        flag_ptr->SetValueFromString(value);
        success=true;
    }
    return success;
}
bool set_quic_cert_key(const std::string& quic_cert_path){
    std::string certificate_file("certificate_file");
    std::string certificate_file_path=quic_cert_path+std::string("leaf_cert.pem");
    std::string key_file("key_file");
    std::string key_file_path=quic_cert_path+std::string("leaf_cert.pkcs8");
    bool success1=false,success2=false;
    success1=(access(certificate_file_path.c_str(), F_OK ) != -1);
    QUIC_LOG_IF(ERROR,!success1)<<certificate_file_path<<" not found";
    success2=(access(key_file_path.c_str(), F_OK ) != -1);
    QUIC_LOG_IF(ERROR,!success2)<<key_file_path<<" not found";
    if(success1&&success2){
        set_quic_key_value(certificate_file,certificate_file_path);
        set_quic_key_value(key_file,key_file_path);
    }
    return success1&&success2;
}
}
