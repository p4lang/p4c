/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <iosfwd>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "lib/compile_context.h"
#include "lib/error_reporter.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

/// Generates a test P4 program for the specified standard metadata field.
static std::optional<TofinoPipeTestCase> makeTestCase(const std::string &field, int bitwidth,
                                                      const std::string &gress) {
    SCOPED_TRACE("V1ModelStdMetaTranslateTest");

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1 { bit<%BITWIDTH%> f; }
        struct H { H1 h1; }
        struct M { }
        parser parse(packet_in packet, out H headers, inout M meta, inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                transition accept;
            }
        }
        control ig(inout H headers, inout M meta, inout standard_metadata_t sm) {
            apply { %INGRESS_ASSIGN% } }
        control eg(inout H headers, inout M meta, inout standard_metadata_t sm) {
            apply { %EGRESS_ASSIGN% } }
        control verifyChecksum(inout H headers, inout M meta) { apply { } }
        control computeChecksum(inout H headers, inout M meta) { apply { } }
        control deparse(packet_out packet, in H headers) {
            apply { packet.emit(headers.h1); } }
        V1Switch(parse(), verifyChecksum(), ig(), eg(),
                 computeChecksum(), deparse()) main;
    )");

    // Our target field is always 48-bit (byte-aligned), we insert a cast for the assignment
    int targetBitwidth(48);
    (void)bitwidth;
    boost::replace_first(source, "%BITWIDTH%", std::to_string(targetBitwidth));
    std::string leftSide("headers.h1.f = (bit<" + std::to_string(targetBitwidth) + ">)");
    std::string empty("");
    if (gress == "ingress")
        boost::replace_first(source, "%INGRESS_ASSIGN%", leftSide + field + ";");
    else
        boost::replace_first(source, "%INGRESS_ASSIGN%", empty);
    if (gress == "egress")
        boost::replace_first(source, "%EGRESS_ASSIGN%", leftSide + field + ";");
    else
        boost::replace_first(source, "%EGRESS_ASSIGN%", empty);

    return TofinoPipeTestCase::create(source);
}

using ::testing::WithParamInterface;

class V1ModelStdMetaTranslateTestBase : public TofinoBackendTest {
 public:
    bool errorOutputContains(const std::string &s) const {
        auto out = errorOutputStream.str();
        return (out.find(s) != std::string::npos);
    }

 private:
    void SetUp() override {
        auto &context = BaseCompileContext::get();
        savedErrorOutputStream = context.errorReporter().getOutputStream();
        context.errorReporter().setOutputStream(&errorOutputStream);
    }

    void TearDown() override {
        auto &context = BaseCompileContext::get();
        context.errorReporter().setOutputStream(savedErrorOutputStream);
    }

    std::stringstream errorOutputStream;
    std::ostream *savedErrorOutputStream;
};

class V1ModelStdMetaTranslateTest
    : public V1ModelStdMetaTranslateTestBase,
      public WithParamInterface<std::tuple<const char *, int, const char *>> {
 public:
    V1ModelStdMetaTranslateTest()
        : field(std::get<0>(GetParam())),
          bitwidth(std::get<1>(GetParam())),
          gress(std::get<2>(GetParam())) {}

 protected:
    const std::string field;  /// standard metadata field name
    const int bitwidth;       /// field bitwidth
    const std::string gress;  /// the "gress" (ingress or egress) in which the field is first valid
};

TEST_P(V1ModelStdMetaTranslateTest, SameGress) {
    auto test = makeTestCase(field, bitwidth, gress);
    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
}

TEST_P(V1ModelStdMetaTranslateTest, OtherGress) {
    const std::string other_gress = (gress == "ingress") ? "egress" : "ingress";
    auto test = makeTestCase(field, bitwidth, other_gress);
    if (gress == "ingress") {
        ASSERT_TRUE(test);
        EXPECT_EQ(0u, ::diagnosticCount());
    } else {
        ASSERT_FALSE(test);
        EXPECT_TRUE(errorOutputContains("is not accessible in the ingress pipe"));
    }
}

INSTANTIATE_TEST_CASE_P(TranslatedFields, V1ModelStdMetaTranslateTest,
                        ::testing::Values(
                            // translation needs to insert a cast but doesn't right now
                            // std::make_tuple("sm.enq_timestamp", 32, "egress"),
                            std::make_tuple("sm.deq_qdepth", 19, "egress"),
                            std::make_tuple("sm.ingress_global_timestamp", 48, "ingress")));

class V1ModelStdMetaTranslateNegativeTest : public V1ModelStdMetaTranslateTestBase {};

TEST_F(V1ModelStdMetaTranslateNegativeTest, DeqTimedelta) {
    auto test = makeTestCase("sm.deq_timedelta", 32, "egress");
    ASSERT_FALSE(test);
    EXPECT_TRUE(errorOutputContains(
        "standard_metadata field standard_metadata.deq_timedelta cannot be translated"));
}

static std::optional<TofinoPipeTestCase> makeTestCaseIngress(
    const std::string &ingress_apply_block) {
    SCOPED_TRACE("V1ModelStdMetaTranslateIngressExitTest");

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        struct H { }
        struct M { bit<8> m1; }
        parser parse(packet_in packet, out H headers, inout M meta, inout standard_metadata_t sm) {
            state start {
                transition accept;
            }
        }
        control ig(inout H headers, inout M meta, inout standard_metadata_t sm) {
            apply { %INGRESS_APPLY_BLOCK% } }
        control eg(inout H headers, inout M meta, inout standard_metadata_t sm) { apply { } }
        control verifyChecksum(inout H headers, inout M meta) { apply { } }
        control computeChecksum(inout H headers, inout M meta) { apply { } }
        control deparse(packet_out packet, in H headers) { apply { packet.emit(headers); } }
        V1Switch(parse(), verifyChecksum(), ig(), eg(), computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS_APPLY_BLOCK%", ingress_apply_block);

    return TofinoPipeTestCase::create(source);
}

class V1ModelStdMetaTranslateIngressExitTest : public V1ModelStdMetaTranslateTestBase,
                                               public WithParamInterface<std::string> {
 public:
    V1ModelStdMetaTranslateIngressExitTest() : ingress_apply_block(GetParam()) {}

 protected:
    const std::string ingress_apply_block;
};

TEST_P(V1ModelStdMetaTranslateIngressExitTest, IngressExitStatement) {
    auto test = makeTestCaseIngress(ingress_apply_block);
    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
}

INSTANTIATE_TEST_CASE_P(IngressExitStatementTest, V1ModelStdMetaTranslateIngressExitTest,
                        ::testing::Values(
                            R"(
            exit;
        )",
                            R"(
            if (sm.ingress_port == 1) {
                exit;
            }
        )",
                            R"(
            if (sm.ingress_port == 1) {
                meta.m1 = 0;
            } else {
                exit;
            }
        )",
                            R"(
            if (sm.ingress_port == 1) {
                meta.m1 = 0;
            }
            exit;
        )"));

}  // namespace P4::Test
