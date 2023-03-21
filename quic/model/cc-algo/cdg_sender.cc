// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cdg_sender.h"
#include "../ns3-quic-private.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "gquiche/quic/core/congestion_control/prr_sender.h"
#include "gquiche/quic/core/congestion_control/rtt_stats.h"
#include "gquiche/quic/core/crypto/crypto_protocol.h"
#include "gquiche/quic/core/quic_constants.h"
#include "gquiche/quic/platform/api/quic_bug_tracker.h"
#include "gquiche/quic/platform/api/quic_flags.h"
#include "gquiche/quic/platform/api/quic_logging.h"

namespace quic {

namespace {
// Constants based on TCP defaults.
const QuicByteCount kMaxBurstBytes = 3 * kDefaultTCPMSS;
const float kCdgBeta = 0.707f;  // cdg backoff factor.
// The minimum cwnd based on RFC 3782 (TCP NewReno) for cwnd reductions on a
// fast retransmission.
const QuicByteCount kDefaultMinimumCongestionWindow = 2 * kDefaultTCPMSS;
bool g_use_tolerance = false;
}  // namespace

extern "C"{
#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	int  __x = x;				\
	int  __d = divisor;			\
	(((int(x))-1) > 0 ||				\
	 ((int(divisor))-1) > 0 || (__x) > 0) ?	\
		(((__x) + ((__d) / 2)) / (__d)) :	\
		(((__x) - ((__d) / 2)) / (__d));	\
}							\
)

int div_round_closest(int a,int b){
    a=DIV_ROUND_CLOSEST(a,b);
    return a;
}
}
static uint32_t nexp_u32(uint32_t ux)
{
  static const uint16_t v[] = {
    /* exp(-x)*65536-1 for x = 0, 0.000256, 0.000512, ... */
        65535,65518, 65501, 65468, 65401, 65267, 65001, 64470, 63422,
        61378, 57484, 50423, 38795, 22965, 8047, 987,14,
        };
  uint64_t res;
  uint32_t msb = ux >> 8;
  int i;
  /* Cut off when ux >= 2^24 (actual result is <= 222/U32_MAX). */
  if (msb > UINT16_MAX)
    return 0;
  /* Scale first eight bits linearly: */
  res = UINT32_MAX - (ux & 0xff) * (UINT32_MAX / 1000000);

  /* Obtain e^(x + y + ...) by computing e^x * e^y * ...: */
  for (i = 1; msb; i++, msb >>= 1) {
    uint64_t y = v[i & -(msb & 1)] + 1ULL;
    res = (res * y) >> 16;
  }

  return (uint32_t)res;
}

TcpCdgSenderBytes::TcpCdgSenderBytes(
    const QuicClock* clock, const RttStats* rtt_stats,
    QuicPacketCount initial_tcp_congestion_window,
    QuicPacketCount max_congestion_window,
    QuicRandom* random,
    QuicConnectionStats* stats)
    : rtt_stats_(rtt_stats),
      random_(random),
      stats_(stats),
      min4_mode_(false),
      last_cutback_exited_slowstart_(false),
      slow_start_large_reduction_(false),
      no_prr_(false),
      num_acked_packets_(0),
      congestion_window_(initial_tcp_congestion_window * kDefaultTCPMSS),
      min_congestion_window_(kDefaultMinimumCongestionWindow),
      max_congestion_window_(max_congestion_window * kDefaultTCPMSS),
      slowstart_threshold_(max_congestion_window * kDefaultTCPMSS),
      initial_tcp_congestion_window_(initial_tcp_congestion_window *
                                     kDefaultTCPMSS),
      initial_max_tcp_congestion_window_(max_congestion_window *
                                         kDefaultTCPMSS),
      min_slow_start_exit_window_(min_congestion_window_),
      cong_state_(TCP_CA_Open),
      cdg_state_(CDG_UNKNOWN),
      backoff_factor_(42),
      backoff_count_(0),
      gfilled_(false),
      tail_(0){
        memset(&cdg_rtt_,0,sizeof(struct minmax));
        memset(&cdg_rtt_prev_,0,sizeof(struct minmax));
        memset(&cdg_gsum_,0,sizeof(struct minmax));
        memset(gradients_,0,kRttWin*sizeof(struct minmax));
      }

TcpCdgSenderBytes::~TcpCdgSenderBytes() {}

TcpCdgSenderBytes::DebugState::DebugState(const TcpCdgSenderBytes& sender)
    : min_rtt(sender.rtt_stats_->smoothed_rtt()),
      latest_rtt(sender.rtt_stats_->latest_rtt()),
      smoothed_rtt(sender.rtt_stats_->smoothed_rtt()),
      mean_deviation(sender.rtt_stats_->mean_deviation()),
      bandwidth_est(sender.BandwidthEstimate()) {}

TcpCdgSenderBytes::DebugState::DebugState(const DebugState& state) = default;

void TcpCdgSenderBytes::SetFromConfig(const QuicConfig& config,
                                        Perspective perspective) {
}

void TcpCdgSenderBytes::AdjustNetworkParameters(const NetworkParams& params) {
  if (params.bandwidth.IsZero() || params.rtt.IsZero()) {
    return;
  }
  SetCongestionWindowFromBandwidthAndRtt(params.bandwidth, params.rtt);
}

float TcpCdgSenderBytes::CdgBeta() const {
  // kNConnectionBeta is the backoff factor after loss for our N-connection
  // emulation, which emulates the effective backoff of an ensemble of N
  // TCP-Reno connections on a single loss event. The effective multiplier is
  // computed as:
  return kCdgBeta;
}

void TcpCdgSenderBytes::OnCongestionEvent(
    bool rtt_updated, QuicByteCount prior_in_flight, QuicTime event_time,
    const AckedPacketVector& acked_packets,
    const LostPacketVector& lost_packets) {
  if (rtt_updated) {
    int64_t rtt_ms=rtt_stats_->latest_rtt().ToMilliseconds();
    if (0 == cdg_rtt_.min){
      cdg_rtt_.min=rtt_ms;
    } else {
      cdg_rtt_.min=std::min(int64_t(cdg_rtt_.min),rtt_ms);
    }
    cdg_rtt_.max=std::max(int64_t(cdg_rtt_.max),rtt_ms);
  }
  
  if (rtt_updated && InSlowStart() &&
      hybrid_slow_start_.ShouldExitSlowStart(
          rtt_stats_->latest_rtt(), rtt_stats_->min_rtt(),
          GetCongestionWindow() / kDefaultTCPMSS)) {
    ExitSlowstart();
  }
  for (const LostPacket& lost_packet : lost_packets) {
    OnPacketLost(lost_packet.packet_number, lost_packet.bytes_lost,
                 prior_in_flight);
  }
  bool round_start=false;
  QuicByteCount  congestion_window_prior=GetCongestionWindow();
  for (const AckedPacket& acked_packet : acked_packets) {
    round_start=UpdateRoundTripCounter(acked_packet.packet_number);
    OnPacketAcked(acked_packet.packet_number, acked_packet.bytes_acked,
                  prior_in_flight, event_time);
  }
  
  if(round_start && cdg_rtt_.v64){
    int32_t grad=0;
    if (cdg_rtt_prev_.v64) {
      grad=CalculateGrad();
    }
    
    cdg_rtt_prev_ = cdg_rtt_;
    cdg_rtt_.v64 = 0;
    if (grad >0 && CdgBackoff(grad)) {
      
    }
  }
  
  if (GetCongestionWindow() > congestion_window_prior){
    QuicByteCount inc=GetCongestionWindow()- congestion_window_prior;
    shadow_congestion_window_=std::max(shadow_congestion_window_,shadow_congestion_window_+)
  }
  
}

void TcpCdgSenderBytes::OnPacketAcked(QuicPacketNumber acked_packet_number,
                                        QuicByteCount acked_bytes,
                                        QuicByteCount prior_in_flight,
                                        QuicTime event_time) {
  largest_acked_packet_number_.UpdateMax(acked_packet_number);
  if (InRecovery()) {
    if (!no_prr_) {
      // PRR is used when in recovery.
      prr_.OnPacketAcked(acked_bytes);
    }
    return;
  }
  if (TCP_CA_CWR == cong_state_) {
    cdg_state_ = CDG_UNKNOWN;
    current_round_trip_end_ = largest_sent_packet_number_;
    cdg_rtt_prev_ = cdg_rtt_;
    cdg_rtt_.v64 = 0;
  }
  
  if (TCP_CA_CWR == cong_state_ || TCP_CA_Recovery == cong_state_){
    cong_state_= TCP_CA_Open;
  }
  
  
  MaybeIncreaseCwnd(acked_packet_number, acked_bytes, prior_in_flight,
                    event_time);
  if (InSlowStart()) {
    hybrid_slow_start_.OnPacketAcked(acked_packet_number);
  }
}

void TcpCdgSenderBytes::OnPacketSent(
    QuicTime /*sent_time*/, QuicByteCount /*bytes_in_flight*/,
    QuicPacketNumber packet_number, QuicByteCount bytes,
    HasRetransmittableData is_retransmittable) {
  if (InSlowStart()) {
    ++(stats_->slowstart_packets_sent);
  }

  if (is_retransmittable != HAS_RETRANSMITTABLE_DATA) {
    return;
  }
  if (InRecovery()) {
    // PRR is used when in recovery.
    prr_.OnPacketSent(bytes);
  }
  QUICHE_DCHECK(!largest_sent_packet_number_.IsInitialized() ||
                largest_sent_packet_number_ < packet_number);
  largest_sent_packet_number_ = packet_number;
  hybrid_slow_start_.OnPacketSent(packet_number);
}

bool TcpCdgSenderBytes::CanSend(QuicByteCount bytes_in_flight) {
  if (!no_prr_ && InRecovery()) {
    // PRR is used when in recovery.
    return prr_.CanSend(GetCongestionWindow(), bytes_in_flight,
                        GetSlowStartThreshold());
  }
  if (GetCongestionWindow() > bytes_in_flight) {
    return true;
  }
  if (min4_mode_ && bytes_in_flight < 4 * kDefaultTCPMSS) {
    return true;
  }
  return false;
}

QuicBandwidth TcpCdgSenderBytes::PacingRate(
    QuicByteCount /* bytes_in_flight */) const {
  // We pace at twice the rate of the underlying sender's bandwidth estimate
  // during slow start and 1.25x during congestion avoidance to ensure pacing
  // doesn't prevent us from filling the window.
  QuicTime::Delta srtt = rtt_stats_->SmoothedOrInitialRtt();
  const QuicBandwidth bandwidth =
      QuicBandwidth::FromBytesAndTimeDelta(GetCongestionWindow(), srtt);
  return bandwidth * (InSlowStart() ? 2 : (no_prr_ && InRecovery() ? 1 : 1.25));
}

QuicBandwidth TcpCdgSenderBytes::BandwidthEstimate() const {
  QuicTime::Delta srtt = rtt_stats_->smoothed_rtt();
  if (srtt.IsZero()) {
    // If we haven't measured an rtt, the bandwidth estimate is unknown.
    return QuicBandwidth::Zero();
  }
  return QuicBandwidth::FromBytesAndTimeDelta(GetCongestionWindow(), srtt);
}

bool TcpCdgSenderBytes::InSlowStart() const {
  return GetCongestionWindow() < GetSlowStartThreshold();
}

bool TcpCdgSenderBytes::IsCwndLimited(QuicByteCount bytes_in_flight) const {
  const QuicByteCount congestion_window = GetCongestionWindow();
  if (bytes_in_flight >= congestion_window) {
    return true;
  }
  const QuicByteCount available_bytes = congestion_window - bytes_in_flight;
  const bool slow_start_limited =
      InSlowStart() && bytes_in_flight > congestion_window / 2;
  return slow_start_limited || available_bytes <= kMaxBurstBytes;
}

bool TcpCdgSenderBytes::InRecovery() const {
  return largest_acked_packet_number_.IsInitialized() &&
         largest_sent_at_last_cutback_.IsInitialized() &&
         largest_acked_packet_number_ <= largest_sent_at_last_cutback_;
}

void TcpCdgSenderBytes::OnRetransmissionTimeout(bool packets_retransmitted) {
  largest_sent_at_last_cutback_.Clear();
  if (!packets_retransmitted) {
    return;
  }
  hybrid_slow_start_.Restart();
  HandleRetransmissionTimeout();
}

std::string TcpCdgSenderBytes::GetDebugState() const {
  return "";
}

TcpCdgSenderBytes::DebugState TcpCdgSenderBytes::ExportDebugState() const {
  return DebugState(*this);
}

void TcpCdgSenderBytes::OnApplicationLimited(
    QuicByteCount /*bytes_in_flight*/) {}

void TcpCdgSenderBytes::SetCongestionWindowFromBandwidthAndRtt(
    QuicBandwidth bandwidth, QuicTime::Delta rtt) {
  QuicByteCount new_congestion_window = bandwidth.ToBytesPerPeriod(rtt);
  // Limit new CWND if needed.
  congestion_window_ =
      std::max(min_congestion_window_,
               std::min(new_congestion_window,
                        kMaxResumptionCongestionWindow * kDefaultTCPMSS));
}

void TcpCdgSenderBytes::SetInitialCongestionWindowInPackets(
    QuicPacketCount congestion_window) {
  congestion_window_ = congestion_window * kDefaultTCPMSS;
}

void TcpCdgSenderBytes::SetExtraLossThreshold(float extra_loss_threshold) {}

void TcpCdgSenderBytes::SetUpdateRangeTime(QuicTime::Delta update_range_time) {}

void TcpCdgSenderBytes::SetIsUpdatePacketLostFlag(bool is_update_min_packet_lost) {}

void TcpCdgSenderBytes::SetUseBandwidthListFlag(bool is_use_bandwidth_list) {}

void TcpCdgSenderBytes::SetMinCongestionWindowInPackets(
    QuicPacketCount congestion_window) {
  min_congestion_window_ = congestion_window * kDefaultTCPMSS;
}

void TcpCdgSenderBytes::SetNumEmulatedConnections(int num_connections) {

}

void TcpCdgSenderBytes::ExitSlowstart() {
  slowstart_threshold_ = congestion_window_;
}

void TcpCdgSenderBytes::OnPacketLost(QuicPacketNumber packet_number,
                                       QuicByteCount lost_bytes,
                                       QuicByteCount prior_in_flight) {
  // TCP NewReno (RFC6582) says that once a loss occurs, any losses in packets
  // already sent should be treated as a single loss event, since it's expected.
  if (largest_sent_at_last_cutback_.IsInitialized() &&
      packet_number <= largest_sent_at_last_cutback_) {
    if (last_cutback_exited_slowstart_) {
      ++stats_->slowstart_packets_lost;
      stats_->slowstart_bytes_lost += lost_bytes;
      if (slow_start_large_reduction_) {
        // Reduce congestion window by lost_bytes for every loss.
        congestion_window_ = std::max(congestion_window_ - lost_bytes,
                                      min_slow_start_exit_window_);
        slowstart_threshold_ = congestion_window_;
      }
    }
    QUIC_DVLOG(1) << "Ignoring loss for largest_missing:" << packet_number
                  << " because it was sent prior to the last CWND cutback.";
    return;
  }
  ++stats_->tcp_loss_events;
  last_cutback_exited_slowstart_ = InSlowStart();
  if (InSlowStart()) {
    ++stats_->slowstart_packets_lost;
  }

  if (!no_prr_) {
    prr_.OnPacketLost(prior_in_flight);
  }

  // TODO(b/77268641): Separate out all of slow start into a separate class.
  if (slow_start_large_reduction_ && InSlowStart()) {
    QUICHE_DCHECK_LT(kDefaultTCPMSS, congestion_window_);
    if (congestion_window_ >= 2 * initial_tcp_congestion_window_) {
      min_slow_start_exit_window_ = congestion_window_ / 2;
    }
    congestion_window_ = congestion_window_ - kDefaultTCPMSS;
  } else{
    congestion_window_ = CdgGetSlowThreshold();
  }
  
  if (congestion_window_ < min_congestion_window_) {
    congestion_window_ = min_congestion_window_;
  }
  
  cong_state_ = TCP_CA_Recovery;
  
  slowstart_threshold_ = congestion_window_;
  largest_sent_at_last_cutback_ = largest_sent_packet_number_;
  // Reset packet count from congestion avoidance mode. We start counting again
  // when we're out of recovery.
  num_acked_packets_ = 0;
  QUIC_DVLOG(1) << "Incoming loss; congestion window: " << congestion_window_
                << " slowstart threshold: " << slowstart_threshold_;
}

QuicByteCount TcpCdgSenderBytes::CdgGetSlowThreshold(){
    if (CDG_BACKOFF == cdg_state_) {
      return  congestion_window_ * CdgBeta();
    }
    if (CDG_NONFULL == cdg_state_ && use_tolerance) {
      return congestion_window_;
    }
    
    shadow_congestion_window_ = std::min (congestion_window_,shadow_congestion_window_ / 2);
    if (use_shadow_) {
      return  std::max(std::max (min_congestion_window_,shadow_congestion_window_), congestion_window_/ 2 )
    }
    return std::max(min_congestion_window_ , congestion_window_/ 2 );
}

QuicByteCount TcpCdgSenderBytes::GetCongestionWindow() const {
  return congestion_window_;
}

QuicByteCount TcpCdgSenderBytes::GetSlowStartThreshold() const {
  return slowstart_threshold_;
}

// Called when we receive an ack. Normal TCP tracks how many packets one ack
// represents, but quic has a separate ack for each packet.
void TcpCdgSenderBytes::MaybeIncreaseCwnd(
    QuicPacketNumber /*acked_packet_number*/, QuicByteCount acked_bytes,
    QuicByteCount prior_in_flight, QuicTime event_time) {
  QUIC_BUG_IF(quic_bug_10439_1, InRecovery())
      << "Never increase the CWND during recovery.";
  // Do not increase the congestion window unless the sender is close to using
  // the current window.
  if (!IsCwndLimited(prior_in_flight)) {
    cubic_.OnApplicationLimited();
    return;
  }
  if (congestion_window_ >= max_congestion_window_) {
    return;
  }
  if (InSlowStart()) {
    // TCP slow start, exponential growth, increase by one for each ACK.
    congestion_window_ += kDefaultTCPMSS;
    QUIC_DVLOG(1) << "Slow start; congestion window: " << congestion_window_
                  << " slowstart threshold: " << slowstart_threshold_;
    return;
  }
  // Congestion avoidance.
  // Classic Reno congestion avoidance.
  ++num_acked_packets_;
  // Divide by num_connections to smoothly increase the CWND at a faster rate
  // than conventional Reno.
  if (num_acked_packets_ >= congestion_window_ / kDefaultTCPMSS) {
    congestion_window_ += kDefaultTCPMSS;
    num_acked_packets_ = 0;
  }

  QUIC_DVLOG(1) << "Reno; congestion window: " << congestion_window_
                << " slowstart threshold: " << slowstart_threshold_
                << " congestion window count: " << num_acked_packets_;

}

void TcpCdgSenderBytes::HandleRetransmissionTimeout() {
  cubic_.ResetCubicState();
  slowstart_threshold_ = congestion_window_ / 2;
  congestion_window_ = min_congestion_window_;
}
bool TcpCdgSenderBytes::UpdateRoundTripCounter(QuicPacketNumber last_acked_packet) {
  if (!current_round_trip_end_.IsInitialized() ||
      last_acked_packet > current_round_trip_end_) {
    current_round_trip_end_ = largest_sent_packet_number_;
    return true;
  }
  return false;
}
int32_t TcpCdgSenderBytes::CalculateGrad(){
  int grag;
  int32_t gmin=cdg_rtt_.min - cdg_rtt_prev_.min;
  int32_t gmax=cdg_rtt_.max - cdg_rtt_prev_.max;
  cdg_gsum_.min += gmin - gradients_[tail_].min;
  cdg_gsum_.max += gmax - gradients_[tail_].max;
  gradients_[tail_].min = gmin;
  gradients_[tail_].max = gmax;
  tail_ = (tail_ + 1) & (kRttWin -1);
  
  gmin = cdg_gsum_.min;
  gmax = cdg_gsum_.max;
  
  grad = gmin > 0 ? gmin : gmax;
  if (!gfilled_) {
    if (0 == tail_ ){
      gfilled_=true;
    }else {
      grad = (grad * kRttWin) / tail_ ;
    }
  }
  
	/* Backoff was effectual: */
  if (gmin <=0 || gmax<=0) {
    backoff_count_=0;
  }
  
  if (g_use_tolerance){
    if (gmin > 0 && gmax <= 0) {
      cdg_state_ = CDG_FULL;
    }else if (gmin > 0 && gmax > 0 || gmax < 0){
      cdg_state_ = CDG_NONFULL;
    }
  }
  
  return grad;
}

bool TcpCdgSenderBytes::CdgBackoff(int32_t grad){
  uint32_t key=0;
  random_->RandBytes((void*)&key,sizeof(key));
  if (key <= nexp_u32(grad * backoff_factor_)) {
    return false;
  }
  
  if (use_ineff_) {
    backoff_count_ ++;
    if (backoff_count_ > use_ineff_) {
      return false;
    }
  }
  shadow_congestion_window_ = std::max(shadow_congestion_window_, congestion_window_);
  cdg_state_ = CDG_BACKOFF
  cong_state_ = TCP_CA_CWR;
  EnterCongestionWindowReduction();
  return true;
}

void TcpCdgSenderBytes::EnterCongestionWindowReduction(){
  cong_state_ = TCP_CA_CWR;
  if (!no_prr_) {
    prr_.OnPacketLost(prior_in_flight);
  }
  congestion_window_ = CdgGetSlowThreshold();
  slowstart_threshold_ = congestion_window_;
  largest_sent_at_last_cutback_ = largest_sent_packet_number_;
  // Reset packet count from congestion avoidance mode. We start counting again
  // when we're out of recovery.
  num_acked_packets_ = 0;
}
void TcpCdgSenderBytes::OnConnectionMigration() {
  hybrid_slow_start_.Restart();
  prr_ = PrrSender();
  largest_sent_packet_number_.Clear();
  largest_acked_packet_number_.Clear();
  largest_sent_at_last_cutback_.Clear();
  last_cutback_exited_slowstart_ = false;
  cubic_.ResetCubicState();
  num_acked_packets_ = 0;
  congestion_window_ = initial_tcp_congestion_window_;
  max_congestion_window_ = initial_max_tcp_congestion_window_;
  slowstart_threshold_ = initial_max_tcp_congestion_window_;
}

CongestionControlType TcpCdgSenderBytes::GetCongestionControlType() const {
  return reno_ ? kRenoBytes : kCubicBytes;
}

}  // namespace quic
