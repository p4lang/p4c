#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf_ir.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace Test {

TEST(P4TestgenOutputOptionTest, GenerateOuputsCorrectly) {
    std::stringstream streamTest;
    streamTest << R"p4(
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
control ingress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
  apply {
      if (hdr.eth_hdr.dst_addr == 0xDEADDEADDEAD && hdr.eth_hdr.src_addr == 0xBEEFBEEFBEEF && hdr.eth_hdr.ether_type == 0xF00D) {
          mark_to_drop(sm);
      }
  }
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

    auto source = P4_SOURCE(P4Headers::V1MODEL, streamTest.str().c_str());
    auto compilerOptions = P4CContextWithOptions<CompilerOptions>::get().options();
    compilerOptions.target = "bmv2";
    compilerOptions.arch = "v1model";
    auto &testgenOptions = P4Tools::P4Testgen::TestgenOptions::get();
    testgenOptions.testBackend = "PROTOBUF_IR";
    testgenOptions.testBaseName = "dummy";
    testgenOptions.seed = 1;
    testgenOptions.maxTests = 0;
    // Create a bespoke packet for the Ethernet extract call.
    testgenOptions.minPktSize = 112;
    testgenOptions.maxPktSize = 112;

    // In this test we only generate tests with dropped packets.
    {
        testgenOptions.droppedPacketOnly = true;

        auto testListOpt =
            P4Tools::P4Testgen::Testgen::generateTests(source, compilerOptions, testgenOptions);

        ASSERT_TRUE(testListOpt.has_value());
        auto testList = testListOpt.value();
        ASSERT_EQ(testList.size(), 1);
        const auto *protobufIrTest =
            testList[0]->checkedTo<P4Tools::P4Testgen::Bmv2::ProtobufIrTest>();
        EXPECT_THAT(protobufIrTest->getFormattedTest(),
                    testing::Not(testing::HasSubstr(R"(expected_output_packet)")));
    }

    // Here we flip the option and only generate tests with forwarded packets.
    {
        testgenOptions.droppedPacketOnly = false;
        testgenOptions.outputPacketOnly = true;

        auto testListOpt =
            P4Tools::P4Testgen::Testgen::generateTests(source, compilerOptions, testgenOptions);

        ASSERT_TRUE(testListOpt.has_value());
        auto testList = testListOpt.value();
        ASSERT_EQ(testList.size(), 1);
        const auto *protobufIrTest =
            testList[0]->checkedTo<P4Tools::P4Testgen::Bmv2::ProtobufIrTest>();
        EXPECT_THAT(protobufIrTest->getFormattedTest(),
                    testing::HasSubstr(R"(expected_output_packet)"));
    }
}

}  // namespace Test
