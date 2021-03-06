#include <iostream>
#include "ns3-quic-tag.h"
#include "ns3/core-module.h"
namespace ns3{
size_t varint_length(uint64_t number){
    int64_t next=number;
    size_t key=0;
    if(next){
        do{
            next=next/128;
            key++;
        }while(next>0);
    }
    return key;
}
TypeId Ns3QuicTag::GetTypeId (void){
   static TypeId tid = TypeId ("ns3::Ns3QuicTag")
     .SetParent<Tag> ()
     .AddConstructor<Ns3QuicTag> ()
     .AddAttribute ("Time",
                    "time stamp",
                    EmptyAttributeValue (),
                    MakeUintegerAccessor (&Ns3QuicTag::GetTime),
                    MakeUintegerChecker<uint64_t> ())
     .AddAttribute ("Number",
                    "sequence number",
                    EmptyAttributeValue (),
                    MakeUintegerAccessor (&Ns3QuicTag::GetSequence),
                    MakeUintegerChecker<uint64_t> ())                    
   ;
   return tid;    
}
TypeId Ns3QuicTag::GetInstanceTypeId (void) const{
    return GetTypeId ();
} 
uint32_t Ns3QuicTag::GetSerializedSize (void) const {
    return varint_length(seq_)+varint_length(time_);
}
void Ns3QuicTag::Serialize (TagBuffer i) const{
    VarintEncode(i,seq_);
    VarintEncode(i,time_);
}
void Ns3QuicTag::Deserialize (TagBuffer i){
    VarientDecode(i,&seq_);
    VarientDecode(i,&time_);
}
void Ns3QuicTag::VarintEncode(TagBuffer &i,uint64_t value) const{
    char first=0;
    uint64_t next=value;
    if(next){
        do{
            uint8_t byte=0;
            first=next%128;
            next=next/128;
            byte=first;
            if(next>0){
                byte|=128;
            }
            i.WriteU8(byte);
        }while(next>0);
    }    
}
void Ns3QuicTag::VarientDecode(TagBuffer &i,uint64_t *value){
    uint64_t remain=0;
    uint64_t remain_multi=1;
    uint8_t byte=0;
    do{
        byte=i.ReadU8();
        remain+=(byte&127)*remain_multi;
        remain_multi*=128;
    }while(byte&128);
    *value=remain;
}
}
