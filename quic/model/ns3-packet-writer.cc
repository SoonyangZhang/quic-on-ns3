#include "ns3-packet-writer.h"
namespace quic{
Ns3PacketWriter::Ns3PacketWriter(Delegate *delegate):delegate_(delegate){}
Ns3PacketWriter::~Ns3PacketWriter(){
    delegate_->OnWriterDestroy();
}
WriteResult Ns3PacketWriter::WritePacket(const char* buffer,
                        size_t buf_len,
                        const QuicIpAddress& self_address,
                        const QuicSocketAddress& peer_address,
                        PerPacketOptions* options){
    int rc=delegate_->WritePacket(buffer,buf_len,self_address,peer_address);
    return WriteResult(WRITE_STATUS_OK, rc);
}
bool Ns3PacketWriter::IsWriteBlocked() const{
    return write_blocked_;
}
void Ns3PacketWriter::SetWritable(){
     write_blocked_ = false;
}
QuicByteCount Ns3PacketWriter::GetMaxPacketSize(
    const QuicSocketAddress& /*peer_address*/) const {
  return kMaxOutgoingPacketSize;
}
bool Ns3PacketWriter::SupportsReleaseTime() const {
  return false;
}

bool Ns3PacketWriter::IsBatchMode() const {
  return false;
}
QuicPacketBuffer Ns3PacketWriter::GetNextWriteLocation(
    const QuicIpAddress& /*self_address*/,
    const QuicSocketAddress& /*peer_address*/) {
  return {nullptr, nullptr};
}

WriteResult Ns3PacketWriter::Flush() {
  return WriteResult(WRITE_STATUS_OK, 0);
}
}
