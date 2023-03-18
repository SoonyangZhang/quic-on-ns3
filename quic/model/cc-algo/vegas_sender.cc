#include "vegas_sender.h"
#include "../ns3-quic-congestion-factory.h"
#include <limits>
#include <stdexcept>
#include <sstream>
#include <string>

#include "gquiche/quic/core/congestion_control/rtt_stats.h"
#include "gquiche/quic/core/quic_time.h"
#include "gquiche/quic/core/quic_time_accumulator.h"
#include "gquiche/quic/platform/api/quic_bug_tracker.h"
#include "gquiche/quic/platform/api/quic_flag_utils.h"
#include "gquiche/quic/platform/api/quic_flags.h"
#include "gquiche/quic/platform/api/quic_logging.h"

namespace quic{
namespace{
    const QuicByteCount kDefaultMinimumCongestionWindow = 4 *kDefaultTCPMSS;
    const QuicByteCount kAlphaCongestionWindow=2*kDefaultTCPMSS;
    const QuicByteCount kBetaCongestionWindow=4*kDefaultTCPMSS;
    const QuicByteCount kGammaCongestionWindow=1*kDefaultTCPMSS;
    const QuicTime::Delta kMinRTTWindowLength = QuicTime::Delta::FromSeconds(10);
}

VegasSender::VegasSender(QuicTime now,
            const RttStats* rtt_stats,
            const QuicUnackedPacketMap* unacked_packets,
            QuicPacketCount initial_tcp_congestion_window,
            QuicPacketCount max_tcp_congestion_window,
            QuicRandom* random,
            QuicConnectionStats* stats):
rtt_stats_(rtt_stats),
unacked_packets_(unacked_packets),
random_(random),
stats_(stats),
congestion_window_(initial_tcp_congestion_window * kDefaultTCPMSS),
initial_congestion_window_(initial_tcp_congestion_window *kDefaultTCPMSS),
max_congestion_window_(max_tcp_congestion_window * kDefaultTCPMSS),
min_congestion_window_(kDefaultMinimumCongestionWindow),
slowstart_threshold_(max_tcp_congestion_window * kDefaultTCPMSS),
pacing_rate_(QuicBandwidth::Zero()),
baseRTTFilter_(kMinRTTWindowLength.ToMicroseconds(),QuicTime::Delta::Zero(),0),
min_rtt_(QuicTime::Delta::Infinite()),
rtt_count_(0),
num_acked_packets_(0),
vegas_mode_(true){
  if (stats_) {
    // Clear some startup stats if |stats_| has been used by another sender,
    // which happens e.g. when QuicConnection switch send algorithms.
    stats_->slowstart_count = 0;
    stats_->slowstart_duration = QuicTimeAccumulator();
  }
}
VegasSender::~VegasSender(){}

bool VegasSender::InSlowStart() const{
    return GetCongestionWindow() < GetSlowStartThreshold();
}
bool VegasSender::InRecovery() const{
  return largest_acked_packet_number_.IsInitialized() &&
         largest_sent_at_last_cutback_.IsInitialized() &&
         largest_acked_packet_number_ <= largest_sent_at_last_cutback_;
}
bool VegasSender::ShouldSendProbingPacket() const{
    return false;
}
void VegasSender::SetInitialCongestionWindowInPackets(QuicPacketCount congestion_window){
    if(InSlowStart()){
        initial_congestion_window_ = congestion_window * kDefaultTCPMSS;
        congestion_window_ = congestion_window * kDefaultTCPMSS;
    }
}

void VegasSender::OnCongestionEvent(bool rtt_updated,
                         QuicByteCount prior_in_flight,
                         QuicTime event_time,
                         const AckedPacketVector& acked_packets,
                         const LostPacketVector& lost_packets){
  if(acked_packets.size()>0){
    auto vrtt=rtt_stats_->latest_rtt()+QuicTime::Delta::FromMicroseconds(1);
    QuicTime::Delta wall_time=event_time-QuicTime::Zero();
    baseRTTFilter_.Update(vrtt,wall_time.ToMicroseconds());
    if(min_rtt_>vrtt){
        min_rtt_=vrtt;
    }
    rtt_count_++;
  }
  bool before=InRecovery();
  
  for (const LostPacket& lost_packet : lost_packets) {
    OnPacketLost(lost_packet.packet_number, lost_packet.bytes_lost,
                 prior_in_flight);
  }
  for (const AckedPacket& acked_packet : acked_packets) {
    OnPacketAcked(acked_packet.packet_number, acked_packet.bytes_acked,
        prior_in_flight, event_time);
  }
  if(InRecovery()!=before){
      if(InRecovery()){
          //vegas_disable
          vegas_mode_=false;
      }else{
          //vegas_enable
          vegas_mode_=true;
          beg_send_next_=largest_sent_packet_number_;
          min_rtt_=QuicTime::Delta::Infinite();
          rtt_count_=0;
      }
  }
}
void VegasSender::OnPacketSent(QuicTime sent_time,
                    QuicByteCount bytes_in_flight,
                    QuicPacketNumber packet_number,
                    QuicByteCount bytes,
                    HasRetransmittableData is_retransmittable){
  QUICHE_DCHECK(!largest_sent_packet_number_.IsInitialized() ||
         largest_sent_packet_number_ < packet_number);
  largest_sent_packet_number_ = packet_number;
  if(!beg_send_next_.IsInitialized()){
      beg_send_next_=largest_sent_packet_number_;
  }
}
void VegasSender::OnRetransmissionTimeout(bool /*packets_retransmitted*/){
    
}
bool VegasSender::CanSend(QuicByteCount bytes_in_flight){
    return bytes_in_flight<GetCongestionWindow();
}
QuicBandwidth VegasSender::PacingRate(QuicByteCount bytes_in_flight) const{
  QuicTime::Delta srtt = rtt_stats_->SmoothedOrInitialRtt();
  const QuicBandwidth bandwidth =
      QuicBandwidth::FromBytesAndTimeDelta(GetCongestionWindow(), srtt);
      return bandwidth * (InSlowStart() ? 2 : (InRecovery() ? 1 : 1.25));
}
QuicBandwidth VegasSender::BandwidthEstimate() const {
  QuicTime::Delta srtt = rtt_stats_->smoothed_rtt();
  if (srtt.IsZero()) {
    // If we haven't measured an rtt, the bandwidth estimate is unknown.
    return QuicBandwidth::Zero();
  }
  return QuicBandwidth::FromBytesAndTimeDelta(GetCongestionWindow(), srtt);
}
QuicByteCount VegasSender::GetCongestionWindow() const {
    return congestion_window_;
}
QuicByteCount VegasSender::GetSlowStartThreshold() const{
    return slowstart_threshold_;
}
CongestionControlType VegasSender::GetCongestionControlType() const{
    return (CongestionControlType)kVegas;
}
std::string VegasSender::GetDebugState() const{
    return "copa";
}
void VegasSender::OnPacketLost(QuicPacketNumber packet_number,
                                       QuicByteCount lost_bytes,
                                       QuicByteCount prior_in_flight){
    if (largest_sent_at_last_cutback_.IsInitialized() &&packet_number <= largest_sent_at_last_cutback_) {
        return;
    }
    congestion_window_=congestion_window_/2;
    congestion_window_=std::max(congestion_window_,min_congestion_window_);
    slowstart_threshold_ = congestion_window_;
    largest_sent_at_last_cutback_=largest_sent_packet_number_;
    num_acked_packets_ = 0;
}
void VegasSender::OnPacketAcked(QuicPacketNumber acked_packet_number,
                                QuicByteCount acked_bytes,
                                QuicByteCount prior_in_flight,
                                QuicTime event_time){
  largest_acked_packet_number_.UpdateMax(acked_packet_number);
  if(!vegas_mode_){
      //reno mode
      IncreaseCongestionWindowAsReno();
      return ;
  }
  QUICHE_CHECK(beg_send_next_.IsInitialized());
  if(acked_packet_number>=beg_send_next_){
      beg_send_next_=largest_sent_packet_number_;
      if(rtt_count_<=2){
          IncreaseCongestionWindowAsReno();
      }else{
          QuicTime::Delta base_rtt=baseRTTFilter_.GetBest();
          QuicByteCount target_cwnd=congestion_window_*base_rtt.ToMicroseconds()/min_rtt_.ToMicroseconds();
          target_cwnd=((target_cwnd+kDefaultTCPMSS-1)/kDefaultTCPMSS)*kDefaultTCPMSS;
          QUICHE_CHECK(min_rtt_>=base_rtt);
          QuicByteCount diff=congestion_window_*(min_rtt_.ToMicroseconds()-base_rtt.ToMicroseconds())/base_rtt.ToMicroseconds();
          if(diff>kGammaCongestionWindow&&InSlowStart()){
			/* Going too fast. Time to slow down
			 * and switch to congestion avoidance.
			 */

			/* Set cwnd to match the actual rate
			 * exactly:
			 *   cwnd = (actual rate) * baseRTT
			 * Then we add 1 because the integer
			 * truncation robs us of full link
			 * utilization.
			*/
            congestion_window_=std::min(congestion_window_,target_cwnd); 
            slowstart_threshold_=congestion_window_;
          }else if(InSlowStart()){
              congestion_window_ += kDefaultTCPMSS;
          }else{
              if(diff>kBetaCongestionWindow){
                  if(congestion_window_>min_congestion_window_){
                      congestion_window_-=kDefaultTCPMSS;
                  }
              }else if(diff<kAlphaCongestionWindow){
                  congestion_window_+=kDefaultTCPMSS;
              }else{
                  //hold
              }
          }
      }
      min_rtt_=QuicTime::Delta::Infinite();
      rtt_count_=0;
  }else if(InSlowStart()){
      congestion_window_ += kDefaultTCPMSS;
  }
  congestion_window_=std::max(congestion_window_,min_congestion_window_);
}
void VegasSender::IncreaseCongestionWindowAsReno(){
      if(InSlowStart()){
          congestion_window_ += kDefaultTCPMSS;
      }else{
          ++num_acked_packets_;
          if (num_acked_packets_*kDefaultTCPMSS>=congestion_window_) {
            congestion_window_ += kDefaultTCPMSS;
            num_acked_packets_ = 0;
          }
      }
}
}
