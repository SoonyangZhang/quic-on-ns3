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
                           "MaxSize", StringValue (std::to_string(1)+"p"));
    //pointToPoint.SetQueue ("ns3::DropTailQueue");
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
void test_alarm(){
    Ptr<AlarmTest> alalrm_test=CreateObject<AlarmTest>(Seconds(1.0),Seconds(20),Seconds(1),5);
    Simulator::Stop (Seconds (22));
    Simulator::Run();
    Simulator::Destroy ();
}
void test_log(){
    test_quic_log();
}
void test_app(){
    LogComponentEnable("QuicClientApp",LOG_LEVEL_ALL);
    LogComponentEnable("QuicServerApp",LOG_LEVEL_ALL);
    LogComponentEnable("Ns3QuicAlarmEngine",LOG_LEVEL_ALL);
    LogComponentEnable("Ns3QuicBackendBase",LOG_LEVEL_ALL);
    uint64_t bps=1000000;
    uint32_t link_delay=100;//milliseconds;
    uint32_t buffer_delay=200;//ms
    NodeContainer topo=BuildExampleTopo(bps,link_delay,buffer_delay);
    Ptr<Node> h1=topo.Get(0);
    Ptr<Node> h2=topo.Get(1);
    Ptr<QuicClientApp> client_app1= CreateObject<QuicClientApp>();
    Ptr<QuicClientApp> client_app2= CreateObject<QuicClientApp>();
    Ptr<QuicServerApp> server_app=CreateObject<QuicServerApp>();
    h1->AddApplication(client_app1);
    h1->AddApplication(client_app2);
    h2->AddApplication(server_app);
    client_app1->Bind();
    client_app2->Bind();
    server_app->Bind(1234);
    InetSocketAddress addr=server_app->GetLocalAddress();
    client_app1->set_peer(addr.GetIpv4(),addr.GetPort());
    client_app2->set_peer(addr.GetIpv4(),addr.GetPort());
    client_app1->SetStartTime (Seconds (0.001));
    client_app1->SetStopTime (Seconds (10));
    client_app2->SetStartTime (Seconds (0.001));
    client_app2->SetStopTime (Seconds (10));
    server_app->SetStartTime (Seconds (0.001));
    server_app->SetStopTime (Seconds (10));
    Simulator::Stop (Seconds(10+1));
    Simulator::Run ();
    Simulator::Destroy();
}
int main(int argc, char* argv[]){
    set_quic_log_level(quiche::ERROR);
    std::string quic_cert_path("/home/ipcom/zsy/v89/quiche/utils/data/quic-cert/");
    ns3::set_quic_cert_key(quic_cert_path);
    test_app();
    //test_arg_config(argc,argv);
    //test_log();
    //test_proof_source();
    return 0;
}
