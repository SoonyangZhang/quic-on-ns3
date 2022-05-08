#include <iostream>
#include <stdlib.h>
#include <memory>
#include <string>
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/quic-module.h"
#include "ns3/nstime.h"
using namespace ns3;
using namespace quic;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("quic-main");
const uint32_t DEFAULT_PACKET_SIZE = 1500;
static NodeContainer BuildExampleTopo(uint64_t bps,
                                      uint32_t msDelay,
                                      uint32_t msQdelay)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    
    int packets=bufSize/DEFAULT_PACKET_SIZE;
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (std::to_string(5)+"p"));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    TrafficControlHelper pfifoHelper;
    uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue (std::to_string(packets)+"p"));
    pfifoHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue (std::to_string(packets)+"p"));
    pfifoHelper.Install(devices);


    Ipv4AddressHelper address;
    std::string nodeip="10.1.1.0";
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);

/*
    std::string errorModelType = "ns3::RateErrorModel";
    ObjectFactory factory;
    factory.SetTypeId (errorModelType);
    Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
*/
    return nodes;
}
void test_arg_config(int argc, char* argv[]){
    const char* usage="./quic-main --test_value=1234";
    parser_flags(usage,argc,argv);
    quic::print_test_value();
}
void test_app(Time app_start,Time app_stop,uint8_t client_log_flag,
                uint8_t server_log_flag,quic::BackendType type,const std::string &cc_name){
    uint64_t bps=5000000;
    uint32_t link_delay=100;//milliseconds;
    uint32_t buffer_delay=300;//ms
    std::string topo_id("1");
    quic::CongestionControlType cc_type=quic::kCubicBytes;
    std::string algo("cubic");
    if(0==cc_name.compare("bbr")){
        cc_type=quic::kBBR;
        algo=cc_name;
    }
    std::string log_path=std::string("/home/ipcom/zsy/traces/")+algo+"/";
    set_quic_trace_folder(log_path);
    if(!MakePath(log_path)){
        std::cout<<log_path<<" is not right"<<std::endl;
        return ;
    }
    NodeContainer topo=BuildExampleTopo(bps,link_delay,buffer_delay);
    Ptr<Node> h1=topo.Get(0);
    Ptr<Node> h2=topo.Get(1);

    Ptr<QuicServerApp> server_app=CreateObject<QuicServerApp>(type);
    h2->AddApplication(server_app);
    server_app->Bind(1234);
    InetSocketAddress server_addr=server_app->GetLocalAddress();
    server_app->SetStartTime (app_start);
    server_app->SetStopTime (app_stop+Seconds(10));
    std::vector<Ns3QuicClientTrace*> client_traces;
    Ns3QuicServerTraceDispatcher *server_trace=nullptr;
    if(server_log_flag&E_QS_ALL){
        DataRate bottleneck_bw(bps);
        server_trace=new Ns3QuicServerTraceDispatcher(bottleneck_bw,app_start);
        server_trace->LogEnable(topo_id,server_log_flag);
        server_app->set_trace(server_trace);
    }
    //client1
    {
        Ptr<QuicClientApp> client_app= CreateObject<QuicClientApp>(type,cc_type);
        h1->AddApplication(client_app);
        client_app->Bind();
        InetSocketAddress client_addr=client_app->GetLocalAddress();
        client_app->set_peer(server_addr.GetIpv4(),server_addr.GetPort());
        client_app->SetStartTime (app_start);
        client_app->SetStopTime (app_stop);
        Ns3QuicAddressPair addr_pair(client_addr,server_addr);
        if(client_log_flag&E_QC_ALL){
            std::string prefix=topo_id+"_"+Ns3QuicAddressPair2String(addr_pair);
            Ns3QuicClientTrace* trace=new Ns3QuicClientTrace();
            trace->LogEnable(prefix,client_log_flag);
            if(client_log_flag&E_QC_IN_FLIGHT){
                client_app->SetInFlightTraceFun(
                            MakeCallback(&Ns3QuicClientTrace::OnInFlight,trace));
            }
            if(client_log_flag&E_QC_SEND_RATE){
                client_app->SetRateTraceFuc(MakeCallback(&Ns3QuicClientTrace::OnSendRate,trace));
            }
            client_traces.push_back(trace);
        }
        if(server_log_flag&E_QS_ALL){
            server_trace->AddMonitorAddressPair(addr_pair);
        }
    }
    //client2
    {
        Ptr<QuicClientApp> client_app= CreateObject<QuicClientApp>(type,cc_type);
        h1->AddApplication(client_app);
        client_app->Bind();
        InetSocketAddress client_addr=client_app->GetLocalAddress();
        client_app->set_peer(server_addr.GetIpv4(),server_addr.GetPort());
        client_app->SetStartTime (app_start+Seconds(20));
        client_app->SetStopTime (app_stop);
        Ns3QuicAddressPair addr_pair(client_addr,server_addr);
        if(client_log_flag&E_QC_ALL){
            std::string prefix=topo_id+"_"+Ns3QuicAddressPair2String(addr_pair);
            Ns3QuicClientTrace* trace=new Ns3QuicClientTrace();
            trace->LogEnable(prefix,client_log_flag);
            if(client_log_flag&E_QC_IN_FLIGHT){
                client_app->SetInFlightTraceFun(
                            MakeCallback(&Ns3QuicClientTrace::OnInFlight,trace));
            }
            if(client_log_flag&E_QC_SEND_RATE){
                client_app->SetRateTraceFuc(MakeCallback(&Ns3QuicClientTrace::OnSendRate,trace));
            }
            client_traces.push_back(trace);
        }
        if(server_log_flag&E_QS_ALL){
            server_trace->AddMonitorAddressPair(addr_pair);
        }
    }
    //client3
    {
        Ptr<QuicClientApp> client_app= CreateObject<QuicClientApp>(type,cc_type);
        h1->AddApplication(client_app);
        client_app->Bind();
        InetSocketAddress client_addr=client_app->GetLocalAddress();
        client_app->set_peer(server_addr.GetIpv4(),server_addr.GetPort());
        client_app->SetStartTime (app_start+Seconds(40));
        client_app->SetStopTime (app_stop);
        Ns3QuicAddressPair addr_pair(client_addr,server_addr);
        if(client_log_flag&E_QC_ALL){
            std::string prefix=topo_id+"_"+Ns3QuicAddressPair2String(addr_pair);
            Ns3QuicClientTrace* trace=new Ns3QuicClientTrace();
            trace->LogEnable(prefix,client_log_flag);
            if(client_log_flag&E_QC_IN_FLIGHT){
                client_app->SetInFlightTraceFun(
                            MakeCallback(&Ns3QuicClientTrace::OnInFlight,trace));
            }
            if(client_log_flag&E_QC_SEND_RATE){
                client_app->SetRateTraceFuc(MakeCallback(&Ns3QuicClientTrace::OnSendRate,trace));
            }
            client_traces.push_back(trace);
        }
        if(server_log_flag&E_QS_ALL){
            server_trace->AddMonitorAddressPair(addr_pair);
        }
    }
    int last_time=WallTimeMillis();
    Simulator::Stop (app_stop+Seconds(20));
    Simulator::Run ();
    Simulator::Destroy();
    for(auto it=client_traces.begin();it!=client_traces.end();it++){
        Ns3QuicClientTrace* ptr=(*it);
        delete ptr;
    }
    client_traces.clear();
    if(server_trace){
        delete server_trace;
    }
    int delta=WallTimeMillis()-last_time;
    std::cout<<"run time ms: "<<delta<<std::endl;
}
int main(int argc, char* argv[]){
    LogComponentEnable("Ns3QuicBackendBase",LOG_LEVEL_ALL);
    LogComponentEnable("Ns3QuicAlarmEngine",LOG_LEVEL_ALL);
    //LogComponentEnable("Ns3QuicChannelBase",LOG_LEVEL_ALL);
    //LogComponentEnable("QuicClientApp",LOG_LEVEL_ALL);
    //LogComponentEnable("QuicServerApp",LOG_LEVEL_ALL);
    std::string cc_type("cubic");
    CommandLine cmd;
    cmd.AddValue ("cc", "cctype",cc_type);
    cmd.Parse (argc, argv);
    
    set_quic_log_level(quiche::ERROR);
    quic::ContentInit('a');
    std::string quic_cert_path("/home/ipcom/zsy/v89/quiche/utils/data/quic-cert/");
    ns3::set_quic_cert_key(quic_cert_path);
    //HELLO_UNIDI
    //HELLO_BIDI
    //BANDWIDTH
    quic::BackendType type=quic::BackendType::BANDWIDTH;
    uint8_t client_log_flag=E_QC_IN_FLIGHT|E_QC_SEND_RATE;
    uint8_t server_log_flag=E_QS_OWD|E_QS_LOST|E_QS_GOODPUT;
    //RegisterExternalCongestionFactory();
    test_app(Seconds(0.001),Seconds(200),client_log_flag,server_log_flag,type,cc_type);
    return 0;
}
