#pragma once
#include "gquiche/quic/platform/api/quic_socket_address.h"
namespace quic{
class QuicPacketWriter;
class Ns3PacketInCallback{
public:
    virtual ~Ns3PacketInCallback(){}
    virtual void ProcessPacket(const QuicSocketAddress& self_address,
                     const QuicSocketAddress& peer_address,
                     const QuicReceivedPacket& packet)=0;
    virtual void OnRegistration()=0;
    virtual void OnUnregistration()=0;
    virtual void OnShutdown()=0;
};
class Ns3QuicPollServer{
public:
    virtual ~Ns3QuicPollServer(){}
    virtual bool CreateUDPSocketAndBind(QuicSocketAddress server_address,
                                QuicIpAddress bind_to_address,
                                int bind_to_port)=0;
    virtual QuicSocketAddress GetLatestClientAddress() const =0;
    
    virtual void RegisterFD(Ns3PacketInCallback *cb)=0;
    virtual void UnregisterFD(Ns3PacketInCallback *cb)=0;
    virtual QuicPacketWriter* CreateQuicPacketWriter()=0;
    
};
}
