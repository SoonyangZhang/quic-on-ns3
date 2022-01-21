#include "gquiche/common/platform/api/quiche_logging.h"
#include "gquiche/quic/core/crypto/quic_random.h"

#include "ns3-quic-connection-helper.h"
#include "ns3-quic-clock.h"
namespace quic{
Ns3QuicConnectionHelper::Ns3QuicConnectionHelper(QuicAllocatorType allocator_type):
random_generator_(QuicRandom::GetInstance()),
clock_(Ns3QuicClock::Instance()),
allocator_type_(allocator_type){}
Ns3QuicConnectionHelper::~Ns3QuicConnectionHelper()=default;
const QuicClock* Ns3QuicConnectionHelper::GetClock() const{
    return clock_;
}

QuicRandom* Ns3QuicConnectionHelper::GetRandomGenerator(){
    return random_generator_;
}
QuicBufferAllocator* Ns3QuicConnectionHelper::GetStreamSendBufferAllocator(){
    if (allocator_type_ == QuicAllocatorType::BUFFER_POOL) {
        return &stream_buffer_allocator_;
    } else {
        QUICHE_DCHECK(allocator_type_ == QuicAllocatorType::SIMPLE);
        return &simple_buffer_allocator_;
    }
}
}
