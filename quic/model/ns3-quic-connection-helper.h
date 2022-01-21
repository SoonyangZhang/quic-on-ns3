#pragma once
#include "gquiche/quic/core/quic_simple_buffer_allocator.h"
#include "gquiche/quic/platform/api/quic_stream_buffer_allocator.h"
#include "gquiche/quic/core/quic_clock.h"
#include "gquiche/quic/core/quic_connection.h"
namespace quic{
class QuicRandom;
enum class QuicAllocatorType { SIMPLE, BUFFER_POOL };
class Ns3QuicConnectionHelper:public QuicConnectionHelperInterface{
public:
    Ns3QuicConnectionHelper(QuicAllocatorType allocator_type);
    Ns3QuicConnectionHelper(const Ns3QuicConnectionHelper&) = delete;
    Ns3QuicConnectionHelper& operator=(const Ns3QuicConnectionHelper&) =delete;
    ~Ns3QuicConnectionHelper() override;
    
    // QuicConnectionHelperInterface
    const QuicClock* GetClock() const override;
    QuicRandom* GetRandomGenerator() override;
    QuicBufferAllocator* GetStreamSendBufferAllocator() override;
private:
    QuicRandom* random_generator_=nullptr;
    QuicClock *clock_=nullptr;
    // Set up allocators.  They take up minimal memory before use.
    // Allocator for stream send buffers.
    QuicStreamBufferAllocator stream_buffer_allocator_;
    SimpleBufferAllocator simple_buffer_allocator_;
    QuicAllocatorType allocator_type_;
};
}
