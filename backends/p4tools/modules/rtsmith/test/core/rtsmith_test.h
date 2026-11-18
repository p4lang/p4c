#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TEST_CORE_RTSMITH_TEST_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TEST_CORE_RTSMITH_TEST_H_

#include <gtest/gtest.h>

#include <sstream>

#include "backends/p4tools/modules/rtsmith/register.h"
#include "backends/p4tools/modules/rtsmith/rtsmith.h"
#include "backends/p4tools/modules/rtsmith/toolname.h"
#include "test/gtest/helpers.h"

namespace P4::P4Tools::Test {

namespace {

using namespace P4::literals;

class RtSmithTest : public testing::Test {
 public:
    [[nodiscard]] static std::optional<std::unique_ptr<AutoCompileContext>> SetUp(
        std::string_view target, std::string_view archName) {
        P4::P4Tools::RtSmith::registerRtSmithTargets();
        /// Set up the appropriate compile context for RtSmith tests.
        /// TODO: Remove this once options are not initialized statically anymore.
        auto ctxOpt = P4::P4Tools::RtSmith::RtSmithTarget::initializeTarget(
            P4::P4Tools::RtSmith::TOOL_NAME, target, archName);

        if (!ctxOpt.has_value()) {
            return std::nullopt;
        }
        return std::make_unique<AutoCompileContext>(ctxOpt.value());
    }
};

std::string generateTestProgram(const char *ingressBlock) {
    std::stringstream templateString;
    templateString << R"p4(
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}
struct Headers {
  ethernet_t eth_hdr;
}
struct Metadata {  }
parser parse(packet_in pkt, out Headers hdr, inout Metadata m, inout standard_metadata_t sm) {
  state start {
      pkt.extract(hdr.eth_hdr);
      transition accept;
  }
}
control ingress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {)p4"
                   << ingressBlock << R"p4(
}
control egress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
  apply {}
}
control deparse(packet_out pkt, in Headers hdr) {
  apply {
    pkt.emit(hdr.eth_hdr);
  }
}
control verifyChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}
control computeChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}
V1Switch(parse(), verifyChecksum(), ingress(), egress(), computeChecksum(), deparse()) main;
)p4";
    return P4_SOURCE(P4Headers::V1MODEL, templateString.str().c_str());
}

}  // anonymous namespace

}  // namespace P4::P4Tools::Test

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TEST_CORE_RTSMITH_TEST_H_ */
