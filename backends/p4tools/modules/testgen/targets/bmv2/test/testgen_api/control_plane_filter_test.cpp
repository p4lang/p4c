#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf_ir.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace Test {

namespace {

TEST(P4TestgenControlPlaneFilterTest, FiltersControlPlaneEntities) {
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
    // Drop the packet.
    action acl_drop() {
        mark_to_drop(sm);
    }

    table drop_table {
        key = {
            hdr.eth_hdr.dst_addr : ternary @name("dst_eth");
        }
        actions = {
            acl_drop();
            @defaultonly NoAction();
        }
    }

    apply {
        if (hdr.eth_hdr.isValid()) {
            drop_table.apply();
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
    auto compilerOptions = CompilerOptions();
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

    // First, we ensure that tests are generated correctly. We expect two tests.
    // One which exercises action acl_drop and one which exercises the default action, NoAction.
    {
        auto testListOpt =
            P4Tools::P4Testgen::Testgen::generateTests(source, compilerOptions, testgenOptions);

        ASSERT_TRUE(testListOpt.has_value());
        auto testList = testListOpt.value();
        ASSERT_EQ(testList.size(), 2);
        const auto *testWithDrop =
            testList[0]->checkedTo<P4Tools::P4Testgen::Bmv2::ProtobufIrTest>();
        EXPECT_THAT(testWithDrop->getFormattedTest(),
                    testing::Not(testing::HasSubstr(R"(expected_packet)")));
        const auto *testWithForwarding =
            testList[1]->checkedTo<P4Tools::P4Testgen::Bmv2::ProtobufIrTest>();
        EXPECT_THAT(testWithForwarding->getFormattedTest(),
                    testing::HasSubstr(R"(expected_output_packet)"));
    }

    // Now we install a filter.
    // Since we can not generate a config for the table we should only generate one test.
    {
        testgenOptions.skippedControlPlaneEntities = {"ingress.drop_table"};

        auto testListOpt =
            P4Tools::P4Testgen::Testgen::generateTests(source, compilerOptions, testgenOptions);

        ASSERT_TRUE(testListOpt.has_value());
        auto testList = testListOpt.value();
        ASSERT_EQ(testList.size(), 1);
        const auto *test = testList[0];
        const auto *protobufIrTest = test->checkedTo<P4Tools::P4Testgen::Bmv2::ProtobufIrTest>();
        EXPECT_THAT(protobufIrTest->getFormattedTest(),
                    testing::Not(testing::HasSubstr(R"(expected_packet)")));
    }
}

}  // namespace

}  // namespace Test
