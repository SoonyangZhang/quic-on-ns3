#pragma once
#include "gquiche/quic/core/quic_clock.h"
namespace quic{
class Ns3QuicClock:public QuicClock{
public:
    static Ns3QuicClock*Instance();
    Ns3QuicClock();
    
    Ns3QuicClock(const Ns3QuicClock&) = delete;
    Ns3QuicClock& operator=(const Ns3QuicClock&) = delete;
    
    ~Ns3QuicClock() override;
    
    // QuicClock implementation:
    QuicTime ApproximateNow() const override;
    QuicTime Now() const override;
    QuicWallTime WallNow() const override;
};
}
