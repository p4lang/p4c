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

#include "bf-p4c/phv/pragma/pa_container_type.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class PaContainerTypePragmaTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createPaContainerTypePragmaTestCase(const std::string &pragmas) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        %PRAGMAS%
        header H1
        {
            bit<8> f1;
            bit<3> f2;
            bit<13> f3;
            bit<48> f4;
        }
        struct Headers { H1 h1; }
        struct Metadata {
            bit<8> f1;
            bit<3> f2;
        }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
            packet.extract(headers.h1);
            transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control mau(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
            packet.emit(headers.h1);
            }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(),
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

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, PhvInfo &phv,
                                   PragmaContainerType &pa_container_type) {
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(phv),
                                 &pa_container_type};
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(PaContainerTypePragmaTest, Basic) {
    auto test = createPaContainerTypePragmaTestCase(P4_SOURCE(P4Headers::NONE,
                                                              R"(
                         @pa_container_type("ingress", "h1.f3", "normal")
                         @pa_container_type("ingress", "h1.f4", "dark")
                         @pa_container_type("ingress", "h1.f2", "mocha")
                         )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaContainerType pa_container_type(phv);
    runMockPasses(test->pipe, phv, pa_container_type);

    auto *h1_f4 = phv.field("ingress::h1.f4"_cs);
    auto *h1_f3 = phv.field("ingress::h1.f3"_cs);
    auto *h1_f2 = phv.field("ingress::h1.f2"_cs);

    EXPECT_EQ(h1_f4->is_dark_candidate(), true);
    EXPECT_EQ(h1_f4->is_mocha_candidate(), false);
    EXPECT_EQ(h1_f3->is_dark_candidate(), false);
    EXPECT_EQ(h1_f3->is_mocha_candidate(), false);
    EXPECT_EQ(h1_f2->is_dark_candidate(), false);
    EXPECT_EQ(h1_f2->is_mocha_candidate(), true);

    EXPECT_EQ(pa_container_type.required_kind(h1_f4), PHV::Kind::dark);
    EXPECT_EQ(pa_container_type.required_kind(h1_f3), PHV::Kind::normal);
    EXPECT_EQ(pa_container_type.required_kind(h1_f2), PHV::Kind::mocha);
}
}  // namespace P4::Test
