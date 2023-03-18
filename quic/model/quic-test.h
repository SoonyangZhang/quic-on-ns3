#pragma once
#include <memory>
#include <string>
#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
namespace quic{
void set_test_value(const std::string &v);
void print_test_value();
void print_address();
void parser_flags(const char* usage,int argc, const char* const* argv);
void test_quic_log();
void test_proof_source();
}
