#include "gquiche/quic/platform/api/quic_flags.h"
#include "ns3-quic-flags.h"
DEFINE_QUIC_COMMAND_LINE_FLAG(int32_t,
                              test_value,
                              123,
                              "Initial MTU of the connection.");
DEFINE_QUIC_COMMAND_LINE_FLAG(
    std::string,
    quic_version,
    "",
    "QUIC version to speak, e.g. 21. If not set, then all available "
    "versions are offered in the handshake. Also supports wire versions "
    "such as Q043 or T099.");

DEFINE_QUIC_COMMAND_LINE_FLAG(bool,
                              quic_ietf_draft,
                              false,
                              "Use the IETF draft version. This also enables "
                              "required internal QUIC flags.");
DEFINE_QUIC_COMMAND_LINE_FLAG(
    bool,
    force_version_negotiation,
    false,
    "If true, start by proposing a version that is reserved for version "
    "negotiation.");
namespace quic{
int get_test_value(){
    return GetQuicFlag(FLAGS_test_value);
}
std::string get_quic_version(){
    return GetQuicFlag(FLAGS_quic_version);
}
bool get_quic_ietf_draft(){
    return GetQuicFlag(FLAGS_quic_ietf_draft);
}
bool get_force_version_negotiation(){
    return GetQuicFlag(FLAGS_force_version_negotiation);
}
}