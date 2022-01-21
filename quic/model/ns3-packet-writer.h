#pragma once
#include "gquiche/quic/core/quic_packet_writer.h"
namespace quic{
class Ns3PacketWriter: public QuicPacketWriter{
public:
    class Delegate{
        public:
        virtual ~Delegate(){}
        //return write length
        virtual int WritePacket(const char* buffer,
                                size_t buf_len,
                                const QuicIpAddress& self_address,
                                const QuicSocketAddress& peer_address)=0;
        virtual void OnWriterDestroy()=0;
    };
    Ns3PacketWriter(Delegate *delegate);
    ~Ns3PacketWriter() override;
    // QuicPacketWriter
    WriteResult WritePacket(const char* buffer,
                            size_t buf_len,
                            const QuicIpAddress& self_address,
                            const QuicSocketAddress& peer_address,
                            PerPacketOptions* options) override;
    bool IsWriteBlocked() const override;
    void SetWritable() override;
    QuicByteCount GetMaxPacketSize(
        const QuicSocketAddress& peer_address) const override;
    bool SupportsReleaseTime() const override;
    bool IsBatchMode() const override;
    QuicPacketBuffer GetNextWriteLocation(
        const QuicIpAddress& self_address,
        const QuicSocketAddress& peer_address) override;
    WriteResult Flush() override;
private:
    Delegate *delegate_=nullptr;
    bool write_blocked_=false;
};
}
