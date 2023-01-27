#pragma once
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
QUIC_EXPORT_PRIVATE class VegasSender:public SendAlgorithmInterface{
public:
  VegasSender(QuicTime now,
            const RttStats* rtt_stats,
            const QuicUnackedPacketMap* unacked_packets,
            QuicPacketCount initial_tcp_congestion_window,
            QuicPacketCount max_tcp_congestion_window,
            QuicRandom* random,
            QuicConnectionStats* stats);
  VegasSender(const VegasSender&) = delete;
  VegasSender& operator=(const VegasSender&) = delete;
  ~VegasSender() override;

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
  void OnRetransmissionTimeout(bool /*packets_retransmitted*/) override;
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

  void OnPacketLost(QuicPacketNumber largest_loss,
                    QuicByteCount lost_bytes,
                    QuicByteCount prior_in_flight);
  void OnPacketAcked(QuicPacketNumber acked_packet_number,
                    QuicByteCount acked_bytes,
                    QuicByteCount prior_in_flight,
                    QuicTime event_time);
  void IncreaseCongestionWindowAsReno();
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
  // Slow start congestion window in bytes, aka ssthresh.
  QuicByteCount slowstart_threshold_;
  // The current pacing rate of the connection.
  QuicBandwidth pacing_rate_;
  using RTTFilter=WindowedFilter<QuicTime::Delta,MinFilter<QuicTime::Delta>,uint64_t,uint64_t> ;
  RTTFilter baseRTTFilter_;
  QuicTime::Delta min_rtt_;
  int32_t rtt_count_;
  QuicPacketNumber beg_send_next_;
  uint64_t num_acked_packets_;
  bool vegas_mode_;
  
};
}