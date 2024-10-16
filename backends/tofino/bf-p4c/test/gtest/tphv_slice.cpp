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
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

namespace P4::Test {

class TPHVSliceTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createTPHVSliceTestCase() {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1 {
            bit<16> f1;
            bit<16> f2;
            bit<16> f3;
            bit<16> f4;
        }
        struct Headers { H1 h1; }
        struct Metadata { }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta, inout
                     standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control mau(inout Headers headers, inout Metadata meta, inout standard_metadata_t sm) {
            action setf1b1(bit<8> val1, bit<8> val2) {
                headers.h1.f1[7:0] = val1;
                headers.h1.f2[15:8] = val2;
            }
            table t1 {
                key = { headers.h1.f1 : exact; }
                actions = { setf1b1; }
                size = 1024;
            }
            apply { t1.apply(); }
        }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
                packet.emit(headers.h1);
            }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(), computeChecksum(), deparse()) main;
    )");
    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe* runMockPasses(
        const IR::BFN::Pipe* pipe,
        PhvInfo& phv,
        PhvUse& uses) {
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(phv),
        &uses
    };
    return pipe->apply(quick_backend);
}

}   // namespace

TEST_F(TPHVSliceTest, Basic) {
    auto test = createTPHVSliceTestCase();
    ASSERT_TRUE(test);

    PhvInfo phv;
    PhvUse uses(phv);
    runMockPasses(test->pipe, phv, uses);

    auto* f1 = phv.field("ingress::h1.f1"_cs);
    auto* f2 = phv.field("ingress::h1.f2"_cs);
    auto* f3 = phv.field("ingress::h1.f3"_cs);
    auto* f4 = phv.field("ingress::h1.f4"_cs);

    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    ASSERT_TRUE(f3);
    ASSERT_TRUE(f4);

    EXPECT_TRUE(f3->is_tphv_candidate(uses));
    EXPECT_TRUE(f4->is_tphv_candidate(uses));
    EXPECT_FALSE(f1->is_tphv_candidate(uses));
    EXPECT_FALSE(f2->is_tphv_candidate(uses));

    PHV::FieldSlice f1b1(f1, StartLen(0, 8));
    PHV::FieldSlice f1b2(f1, StartLen(8, 8));
    PHV::FieldSlice f2b1(f2, StartLen(0, 8));
    PHV::FieldSlice f2b2(f2, StartLen(8, 8));

    EXPECT_FALSE(f1b1.is_tphv_candidate(uses));
    EXPECT_FALSE(f1b2.is_tphv_candidate(uses));
    EXPECT_TRUE(f2b1.is_tphv_candidate(uses));
    EXPECT_FALSE(f2b2.is_tphv_candidate(uses));
}

}   // namespace P4::Test
