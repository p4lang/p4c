#include "backends/p4tools/modules/rtsmith/test/core/rtsmith_test.h"

namespace P4::P4Tools::Test {

namespace {

using namespace P4::literals;

/// Helper methods to build configurations for Optimization Tests.
class P4RuntimeApiTest : public RtSmithTest {};

TEST_F(P4RuntimeApiTest, RejectsUnknownTarget) {
    auto context = SetUp("bmv2", "v1model");
    ASSERT_TRUE(context);
    auto &options = RtSmith::RtSmithOptions::get();
    options.target = "unknown-target"_cs;
    options.arch = "v1model"_cs;
    auto result = RtSmith::RtSmith::generateConfig(generateTestProgram("apply {}"), options);
    EXPECT_FALSE(result);
    EXPECT_GT(errorCount(), 0U);
}

// Tests for the optimization of various expressions.
TEST_F(P4RuntimeApiTest, GeneratesATestViaTheApi) {
    auto source = generateTestProgram(R"(
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
    })");
    auto autoContext = SetUp("bmv2", "v1model");
    auto &rtSmithOptions = RtSmith::RtSmithOptions::get();
    rtSmithOptions.target = "bmv2"_cs;
    rtSmithOptions.arch = "v1model"_cs;
    auto rtSmithResultOpt = P4::P4Tools::RtSmith::RtSmith::generateConfig(source, rtSmithOptions);
    ASSERT_TRUE(rtSmithResultOpt.has_value());
}

}  // anonymous namespace

}  // namespace P4::P4Tools::Test
