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

#include <optional>
#include <boost/algorithm/string/replace.hpp>
#include "gtest/gtest.h"

#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_atomic.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

namespace P4::Test {

class PaAtomicPragmaTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase>
createPaAtomicPragmaTestCase(const std::string& pragmas) {
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

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe* pipe,
                                   PhvInfo& phv,
                                   PragmaAtomic& pa_atomic) {
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(phv),
        &pa_atomic
    };
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(PaAtomicPragmaTest, Basic) {
    auto test = createPaAtomicPragmaTestCase(
            P4_SOURCE(P4Headers::NONE,
                      R"(
                         @pa_atomic("ingress", "h1.f3")
                         @pa_atomic("ingress", "h1.f4")
                         )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaAtomic pa_atomic(phv);
    runMockPasses(test->pipe, phv, pa_atomic);

    auto* h1_f4 = phv.field("ingress::h1.f4"_cs);
    auto* h1_f3 = phv.field("ingress::h1.f3"_cs);

    EXPECT_EQ(h1_f3->no_split(), true);
    EXPECT_EQ(h1_f4->no_split(), false);  // skip if field greater than 32b
}
}  // namespace P4::Test
