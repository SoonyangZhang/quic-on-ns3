#pragma once
#include "gquiche/quic/core/congestion_control/send_algorithm_interface.h"
namespace quic{
enum CongestionControlType2{
    kCC0=CongestionControlType::kBBRv2,
    kCopa,
    kVegas,
};
}
