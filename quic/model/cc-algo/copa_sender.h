#pragma once
/*
Copa: Practical Delay-Based Congestion Control for the Internet
The implementation is copied from mvfst(https://github.com/facebookincubator/mvfst/).
*/
#include <cstdint>
#include <ostream>
#include <string>

#include "gquiche/quic/core/congestion_control/bandwidth_sampler.h"
#include "gquiche/quic/core/congestion_control/send_algorithm_interface.h"
#include "gquiche/quic/core/congestion_control/windowed_filter.h"
#include "gquiche/quic/core/crypto/quic_random.h"
#include "gquiche/quic/core/quic_bandwidth.h"
#include "gquiche/quic/core/quic_packet_number.h"
#include "gquiche/quic/core/quic_packets.h"
#include "gquiche/quic/core/quic_time.h"
#include "gquiche/quic/core/quic_unacked_packet_map.h"
#include "gquiche/quic/platform/api/quic_export.h"
#include "gquiche/quic/platform/api/quic_flags.h"
namespace quic{
class RttStats;
QUIC_EXPORT_PRIVATE class CopaSender:public SendAlgorithmInterface{
public:
  CopaSender(QuicTime now,
            const RttStats* rtt_stats,
            const QuicUnackedPacketMap* unacked_packets,
            QuicPacketCount initial_tcp_congestion_window,
            QuicPacketCount max_tcp_congestion_window,
            QuicRandom* random,
            QuicConnectionStats* stats);
  CopaSender(const CopaSender&) = delete;
  CopaSender& operator=(const CopaSender&) = delete;
  ~CopaSender() override;

  // Start implementation of SendAlgorithmInterface.
  bool InSlowStart() const override;
  bool InRecovery() const override;
  bool ShouldSendProbingPacket() const override;

  void SetFromConfig(const QuicConfig& config,
                     Perspective perspective) override{}
  void ApplyConnectionOptions(const QuicTagVector& connection_options) override{}

  void AdjustNetworkParameters(const NetworkParams& params) override{}
  void SetInitialCongestionWindowInPackets(
      QuicPacketCount congestion_window) override;
  void OnCongestionEvent(bool rtt_updated,
                         QuicByteCount prior_in_flight,
                         QuicTime event_time,
                         const AckedPacketVector& acked_packets,
                         const LostPacketVector& lost_packets) override;
  void OnPacketSent(QuicTime sent_time,
                    QuicByteCount bytes_in_flight,
                    QuicPacketNumber packet_number,
                    QuicByteCount bytes,
                    HasRetransmittableData is_retransmittable) override;
  void OnPacketNeutered(QuicPacketNumber packet_number) override{}
  void OnRetransmissionTimeout(bool /*packets_retransmitted*/) override {}
  void OnConnectionMigration() override {}
  bool CanSend(QuicByteCount bytes_in_flight) override;
  QuicBandwidth PacingRate(QuicByteCount bytes_in_flight) const override;
  QuicBandwidth BandwidthEstimate() const override;
  QuicByteCount GetCongestionWindow() const override;
  QuicByteCount GetSlowStartThreshold() const override;
  CongestionControlType GetCongestionControlType() const override;
  std::string GetDebugState() const override;
  void OnApplicationLimited(QuicByteCount bytes_in_flight) override{}
  void PopulateConnectionStats(QuicConnectionStats* stats) const override{}
  // End implementation of SendAlgorithmInterface.
  
private:
  struct VelocityState {
    uint64_t velocity{1};
    enum Direction {
      None,
      Up, // cwnd is increasing
      Down, // cwnd is decreasing
    };
    Direction direction{None};
    // number of rtts direction has remained same
    uint64_t numTimesDirectionSame{0};
    // updated every srtt
    QuicByteCount lastRecordedCwndBytes;
    QuicTime lastCwndRecordTime{QuicTime::Zero()};
  };
  void OnPacketLost(QuicPacketNumber largest_loss,
                    QuicByteCount lost_bytes,
                    QuicByteCount prior_in_flight);
  void OnPacketAcked(const AckedPacketVector&acked_packets,
                     QuicByteCount prior_in_flight,
                     QuicTime event_time);
  void CheckAndUpdateDirection(const QuicTime ackTime);
  void ChangeDirection(VelocityState::Direction newDirection,
                        const QuicTime ackTime);
  // Determines the appropriate pacing rate for the connection.
  void CalculatePacingRate();
  
  const RttStats* rtt_stats_;
  const QuicUnackedPacketMap* unacked_packets_;
  QuicRandom* random_;
  QuicConnectionStats* stats_;
  
  // Track the largest packet that has been sent.
  QuicPacketNumber largest_sent_packet_number_;
  // Track the largest packet that has been acked.
  QuicPacketNumber largest_acked_packet_number_;
  // Track the largest packet number outstanding when a CWND cutback occurs.
  QuicPacketNumber largest_sent_at_last_cutback_;
  // The maximum allowed number of bytes in flight.
  QuicByteCount congestion_window_;
  // The initial value of the |congestion_window_|.
  QuicByteCount initial_congestion_window_;
  // The largest value the |congestion_window_| can achieve.
  QuicByteCount max_congestion_window_;
  // The smallest value the |congestion_window_| can achieve.
  QuicByteCount min_congestion_window_;
  // The current pacing rate of the connection.
  QuicBandwidth pacing_rate_;
  bool isSlowStart_;
  QuicTime lastCwndDoubleTime_;
  using RTTFilter=WindowedFilter<QuicTime::Delta,MinFilter<QuicTime::Delta>,uint64_t,uint64_t> ;
  RTTFilter minRTTFilter_;
  RTTFilter standingRTTFilter_;
  VelocityState velocityState_;
  /**
   * latencyFactor_ determines how latency sensitive the algorithm is. Lower
   * means it will maximime throughput at expense of delay. Higher value means
   * it will minimize delay at expense of throughput.
   */
  double latencyFactor_{0.50};
};
}