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

#include "bf-p4c/phv/pragma/pa_byte_pack.h"

#include <exception>
#include <optional>
#include <ostream>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class PaBytePackPragmaTest : public TofinoBackendTest {
 private:
    std::ostream *backup;

 public:
    std::stringstream err_stream;

    void SetUp() {
        // errorReporter is now stable, redirect to stringstream
        backup = BaseCompileContext::get().errorReporter().getOutputStream();
        BaseCompileContext::get().errorReporter().setOutputStream(&err_stream);
    }

    void TearDown() {
        // Restore error stream to original
        BaseCompileContext::get().errorReporter().setOutputStream(backup);
        err_stream.clear();
    }

    std::string get_output() const { return err_stream.str(); }
};

namespace {

std::optional<TofinoPipeTestCase> createPaBytePackPragmaTestCase(const std::string &pragmas) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        %PRAGMAS%
        header H1
        {
            bit<32> op;
            bit<2>  pad1;
            bit<3>  m3_src;
            bit<3>  pad2;
        }
        struct Headers { H1 h1; }
        struct Metadata {
            bit<4> m1;
            bit<1> m_r1;
            bit<6> m2;
            bit<5> m_r2;
            bit<3> m3;
        }

        parser parse(packet_in packet, out Headers headers, inout Metadata md,
                     inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                md.m3 = headers.h1.m3_src;
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control ingress(inout Headers headers, inout Metadata md,
                        inout standard_metadata_t sm) {
            action set_md(bit<4> m1, bit<6> m2, bit<1> m_r1, bit<5> m_r2) {
                md.m1 = m1;
                md.m2 = m2;
                md.m_r1 = m_r1;
                md.m_r2 = m_r2;
            }
            table set_metadata {
                actions = { set_md; }
                key = { headers.h1.op: exact; }
                size = 512;
            }

            action set_h1(bit<32> op) { headers.h1.op = op; }
            table match_metadata {
                actions = { set_h1; }
                key = {
                    md.m1 : exact;
                    md.m2 : exact;
                    md.m3 : exact;
                }
                size = 1024;
            }

            apply {
               set_metadata.apply();
               if (md.m_r1 == 1 && md.m_r2 == 2) {
                   match_metadata.apply();
               }
            }
        }
        control egress(inout Headers headers, inout Metadata meta,
                       inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply { packet.emit(headers.h1); }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%PRAGMAS%", pragmas);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, PhvInfo &phv, PragmaBytePack &pa_bp) {
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(phv), &pa_bp};
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(PaBytePackPragmaTest, BasicCase) {
    auto test =
        createPaBytePackPragmaTestCase(P4_SOURCE(P4Headers::NONE,
                                                 R"(@pa_byte_pack("ingress", "md.m1", "md.m2", 6)
                         @pa_byte_pack("ingress", 1, "md.m_r2", 1, "md.m_r1"))"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaBytePack pa_bp(phv);
    runMockPasses(test->pipe, phv, pa_bp);

    const auto &packings = pa_bp.packings();
    std::stringstream layout_ss;
    for (const auto &pack : packings) {
        EXPECT_FALSE(pack.compiler_added);
        EXPECT_TRUE(pack.src_info.has_value());
        layout_ss << pack.packing;
    }
    EXPECT_EQ(
        "[ingress::md.m1 bit[3..0], ingress::md.m2 bit[5..0], 6]"
        "[1, ingress::md.m_r2 bit[4..0], 1, ingress::md.m_r1 bit[0]]",
        layout_ss.str());
}

TEST_F(PaBytePackPragmaTest, WarnParsedField) {
    auto test = createPaBytePackPragmaTestCase(
        P4_SOURCE(P4Headers::NONE, R"(@pa_byte_pack("ingress", "md.m1", "md.m2", "md.m3", 3))"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaBytePack pa_bp(phv);
    runMockPasses(test->pipe, phv, pa_bp);

    const auto &packings = pa_bp.packings();
    ASSERT_EQ(size_t(1), packings.size());
    const auto &pack = packings.front();
    EXPECT_FALSE(pack.compiler_added);
    EXPECT_TRUE(pack.src_info.has_value());
    std::stringstream layout_ss;
    layout_ss << pack.packing;
    EXPECT_EQ("[ingress::md.m1 bit[3..0], ingress::md.m2 bit[5..0], ingress::md.m3 bit[2..0], 3]",
              layout_ss.str());

    // check warning
    EXPECT_THAT(err_stream.str(),
                testing::HasSubstr("Applying @pa_byte_pack on parsed field ingress::md.m3"));
}

TEST_F(PaBytePackPragmaTest, ErrorHeaderField) {
    auto test = createPaBytePackPragmaTestCase(
        P4_SOURCE(P4Headers::NONE, R"(@pa_byte_pack("ingress", "headers.h1.op"))"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaBytePack pa_bp(phv);
    runMockPasses(test->pipe, phv, pa_bp);

    const auto &packings = pa_bp.packings();
    ASSERT_TRUE(packings.empty());
    // check error
    EXPECT_THAT(err_stream.str(),
                testing::HasSubstr("@pa_byte_pack pragma can only be applied on metadata or pov"
                                   " fields, but ingress::headers.h1.op is not"));
}

TEST_F(PaBytePackPragmaTest, ErrorInvalidTotalSize) {
    auto test = createPaBytePackPragmaTestCase(
        P4_SOURCE(P4Headers::NONE, R"(@pa_byte_pack("ingress", "md.m1", "md.m2", 5))"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaBytePack pa_bp(phv);
    runMockPasses(test->pipe, phv, pa_bp);

    const auto &packings = pa_bp.packings();
    ASSERT_TRUE(packings.empty());

    // check error
    EXPECT_THAT(
        err_stream.str(),
        testing::HasSubstr("invalid @pa_byte_pack pragma: the number of bits (15) mod 8 != 0"));
}

TEST_F(PaBytePackPragmaTest, ErrorInvalidPaddingSize) {
    auto test = createPaBytePackPragmaTestCase(
        P4_SOURCE(P4Headers::NONE, R"(@pa_byte_pack("ingress", "md.m1", "md.m2", 14))"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaBytePack pa_bp(phv);
    runMockPasses(test->pipe, phv, pa_bp);

    const auto &packings = pa_bp.packings();
    ASSERT_TRUE(packings.empty());

    // check error
    EXPECT_THAT(err_stream.str(), testing::HasSubstr("Invalid size of padding"));
}

}  // namespace P4::Test
