#pragma once
#include <string>
namespace quic{
    int get_test_value();
    std::string get_quic_version();
    bool get_quic_ietf_draft();
    bool get_force_version_negotiation();
}