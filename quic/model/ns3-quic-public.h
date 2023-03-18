#pragma once
#include <stdint.h>
namespace quic{
enum class BackendType{
HELLO_UNIDI,
HELLO_BIDI,
BANDWIDTH
};
void ContentInit(const char v);
}
