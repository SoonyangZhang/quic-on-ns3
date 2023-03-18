#include <memory.h>
#include <utility>
#include <chrono>
#include <dirent.h>
#include <unistd.h>// access
#include <sys/stat.h> // stat
#include "gquiche/quic/platform/api/quic_flags.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "gquiche/quic/platform/api/quic_logging.h"
#include "ns3-quic-addr-convert.h"
#include "ns3/ns3-quic-util.h"
namespace ns3{
namespace{
    static bool quic_log_level_setted=false;
    static std::string colon(":");
    static std::string undeline("_");
    static std::string period(".");
}
//https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
static bool IsDirExist(const std::string& path)
{
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else 
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool MakePath(const std::string& path)
{
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
#endif
    if (ret == 0)
        return true;

    switch (errno)
    {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
#if defined(_WIN32)
                pos = path.find_last_of('\\');
            if (pos == std::string::npos)
#endif
                return false;
            if (!MakePath( path.substr(0, pos) ))
                return false;
        }
        // now, try to create again
#if defined(_WIN32)
        return 0 == _mkdir(path.c_str());
#else 
        return 0 == mkdir(path.c_str(), mode);
#endif
    case EEXIST:
        // done!
        return IsDirExist(path);

    default:
        return false;
    }
}

void GetFiles(std::string &cate_dir,std::vector<std::string> &files)
{
    DIR *dir;
    struct dirent *ptr;
    if ((dir=opendir(cate_dir.c_str())) == NULL){
        perror("Open dir error...");
        exit(1);
    }
    
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
                continue;
        else if(ptr->d_type == 8)    ///file
            //printf("d_name:%s/%s\n",basePath,ptr->d_name);
            files.push_back(ptr->d_name);
        else if(ptr->d_type == 10)    ///link file
            //printf("d_name:%s/%s\n",basePath,ptr->d_name);
            continue;
        else if(ptr->d_type == 4)    ///dir
        {
            files.push_back(ptr->d_name);
        }
    }
    closedir(dir);
}

static inline int64_t WallTimeNowInUsec(){
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();    
    std::chrono::microseconds mic = std::chrono::duration_cast<std::chrono::microseconds>(d);
    return mic.count(); 
}

int WallTimeMillis(){
    return WallTimeNowInUsec()/1000;
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

void set_quic_log_error_level(){
    set_quic_log_level(quiche::ERROR);
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

std::string GetNs3Ipv4Address2String(const Ipv4Address& ip){
    uint32_t ip_host_order=ip.Get();
    return GetIp2String(ip_host_order);
}

std::string GetIp2String(uint32_t ip_host_order){
    uint8_t buffer[4];
    buffer[0] = (ip_host_order>> 24) & 0xff;
    buffer[1] = (ip_host_order>> 16) & 0xff;
    buffer[2] = (ip_host_order>> 8) & 0xff;
    buffer[3] = (ip_host_order>> 0) & 0xff;
    std::string ret;
    ret=std::to_string((int)buffer[0])+period+
        std::to_string((int)buffer[1])+period+
        std::to_string((int)buffer[2])+period+
        std::to_string((int)buffer[3]);
    return ret;
}

std::string Ns3QuicAddressPair2String(const Ns3QuicAddressPair& addr_pair){
    std::string ret;
    std::string client_ip=GetIp2String(addr_pair.client_ip);
    std::string server_ip=GetIp2String(addr_pair.server_ip);
    std::string client_port=std::to_string(addr_pair.client_port);
    std::string server_port=std::to_string(addr_pair.server_port);
    ret=client_ip+colon+client_port+undeline+
        server_ip+colon+server_port;
    return ret;
}
}
