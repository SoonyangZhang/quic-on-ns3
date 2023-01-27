#include "gquiche/quic/core/congestion_control/bbr2_sender.h"
#include "gquiche/quic/core/congestion_control/bbr_sender.h"
#include "gquiche/quic/core/congestion_control/tcp_cubic_sender_bytes.h"

#include "ns3-quic-congestion-factory.h"
#include "ns3-quic-no-destructor.h"
#include "cc-algo/copa_sender.h"
namespace quic{
class Ns3QuicCongestionFactory:public AbstractCongestionFactory{
public:
    ~Ns3QuicCongestionFactory() override{}
    SendAlgorithmInterface* Create(
      const QuicClock* clock,
      const RttStats* rtt_stats,
      const QuicUnackedPacketMap* unacked_packets,
      CongestionControlType type,
      QuicRandom* random,
      QuicConnectionStats* stats,
      QuicPacketCount initial_congestion_window,
      QuicPacketCount max_congestion_window,
      SendAlgorithmInterface* old_send_algorithm) override;
};
SendAlgorithmInterface* Ns3QuicCongestionFactory::Create(
    const QuicClock* clock,
    const RttStats* rtt_stats,
    const QuicUnackedPacketMap* unacked_packets,
    CongestionControlType type,
    QuicRandom* random,
    QuicConnectionStats* stats,
    QuicPacketCount initial_congestion_window,
    QuicPacketCount max_congestion_window,
    SendAlgorithmInterface* old_send_algorithm){
    int v=type;
    SendAlgorithmInterface *algo=nullptr;
    if(kBBR==v){
        algo=new BbrSender(clock->ApproximateNow(), rtt_stats, unacked_packets,
                           initial_congestion_window, max_congestion_window,
                           random, stats);
    }else if(kBBRv2==v){
      algo=new Bbr2Sender(clock->ApproximateNow(), rtt_stats, unacked_packets,
          initial_congestion_window, max_congestion_window, random, stats,
          old_send_algorithm &&
          old_send_algorithm->GetCongestionControlType() == kBBR
              ? static_cast<BbrSender*>(old_send_algorithm)
              : nullptr);
    }else if(kCubicBytes==v){
        algo=new TcpCubicSenderBytes(clock, rtt_stats, false /* don't use Reno */,
          initial_congestion_window, max_congestion_window, stats);
    }else if(kCopa==v){
        algo=new CopaSender(clock->ApproximateNow(), rtt_stats, unacked_packets,
                           initial_congestion_window, max_congestion_window,
                           random, stats);
    }else{
        algo=new TcpCubicSenderBytes(clock, rtt_stats, true /* use Reno */,
                                     initial_congestion_window,
                                     max_congestion_window, stats);
    }
    return algo;
}
Ns3QuicCongestionFactory* CreateCongestionFactory() {
  static NoDestructor<Ns3QuicCongestionFactory> singleton;
  return singleton.get();
}
void RegisterExternalCongestionFactory(){
    SetCongestionFactory(CreateCongestionFactory());
}
}
