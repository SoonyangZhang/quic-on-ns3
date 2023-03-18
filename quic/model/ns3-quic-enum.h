#pragma once
#include <stdint.h>
namespace quic{
enum QuicCongestionControlType:uint8_t{
  kQuicCubicBytes,
  kQuicRenoBytes,
  kQuicBBR,
  kQuicPCC,
  kQuicGoogCC,
  kQuicBBRv2,
  kQuicCopa,
  kQuicVegas,
};
enum class BackendType{
HELLO_UNIDI,
HELLO_BIDI,
BANDWIDTH
};
}