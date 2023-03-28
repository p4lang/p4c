#include "backends/p4tools/modules/testgen/test/small-step/util.h"

#include <string>

#include <boost/algorithm/string/replace.hpp>

#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

namespace Test {

namespace SmallStepUtil {

/// Creates a test case with the @hdrFields for stepping on an @expr.
std::optional<const P4ToolsTestCase> createSmallStepExprTest(const std::string &hdrFields,
                                                             const std::string &expr) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
header H {
  %HEADER_FIELDS%
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
    m(%EXPR%);
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

    boost::replace_first(source, "%HEADER_FIELDS%", hdrFields);
    boost::replace_first(source, "%EXPR%", expr);

    return P4ToolsTestCase::create_16("bmv2", "v1model", source);
}

}  // namespace SmallStepUtil

}  // namespace Test
