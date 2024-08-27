#include "backends/p4tools/modules/testgen/targets/bmv2/test/gtest_utils.h"

#include "absl/strings/substitute.h"
#include "test/gtest/helpers.h"

namespace P4::P4Tools::Test {
std::optional<const P4ToolsTestCase> createBmv2V1modelSmallStepExprTest(
    const std::string &hdrFields, const std::string &expr) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
header H {
  $0
}

struct Headers {
  H h;
}

struct Metadata { }

parser parse(packet_in pkt,
             out Headers hdr,
             inout Metadata metadata,
             inout standard_metadata_t sm) {
  state start {
      pkt.extract(hdr.h);
      transition accept;
  }
}

extern void m<T>(in T data);

control mau(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
  apply {
    m($1);
  }
}

control deparse(packet_out pkt, in Headers hdr) {
  apply {
    pkt.emit(hdr.h);
  }
}

control verifyChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}

control computeChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}

V1Switch(parse(), verifyChecksum(), mau(), mau(), computeChecksum(), deparse()) main;)");

    return P4ToolsTestCase::create_16("bmv2", "v1model", absl::Substitute(source, hdrFields, expr));
}

}  // namespace P4::P4Tools::Test
