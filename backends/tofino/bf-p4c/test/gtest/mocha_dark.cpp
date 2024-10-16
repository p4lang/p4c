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
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

#include "bf-p4c/phv/analysis/mocha.h"
#include "bf-p4c/phv/analysis/dark.h"
#include "bf-p4c/phv/analysis/non_mocha_dark_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

namespace P4::Test {

class MochaAnalysisTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase>
createMochaAnalysisTest(const std::string& parserSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
header H1
{
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    @padding bit<7> pad_0;
    bit<1> f4;
}

struct Headers { H1 h1; }

struct Metadata {
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<1> f4;
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
    inout standard_metadata_t sm) {
%MAU%
}

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h1); }
}

V1Switch(parse(), verifyChecksum(), mau(), mau(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%MAU%", parserSource);

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe* pipe,
                                   PhvInfo& phv,
                                   FieldDefUse& defuse,
                                   PhvUse& uses) {
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(phv),
        new DoInstructionSelection(phv),
        &defuse,
        &uses
    };
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(MochaAnalysisTest, AnalyzeMochaCandidates) {
    auto test = createMochaAnalysisTest(
        P4_SOURCE(P4Headers::NONE, R"(
action do1() {
    meta.f1 = headers.h1.f1;
    meta.f2 = headers.h1.f2;
    meta.f3 = headers.h1.f3;
    meta.f4 = headers.h1.f4;
}

action do2() {
    meta.f2 = 2;
}

action do3() {
    headers.h1.f1 = meta.f2;
    headers.h1.f2 = meta.f1;
    headers.h1.f3 = meta.f3;
    headers.h1.f4 = meta.f4;
}

table t1 {
    key = { headers.h1.f1 : exact; }
    actions = { do1; do2; }
    size = 512;
}

table t2 {
    key = { meta.f1 : exact;}
    actions = { do3; }
    size = 512;
}

apply {
    t1.apply();
    t2.apply();
}
)"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    FieldDefUse defuse(phv);
    PhvUse uses(phv);
    ReductionOrInfo red_info;
    PHV::Pragmas pragmas(phv);
    NonMochaDarkFields nonMochaDark(phv, uses, defuse, red_info, pragmas);

    auto* pipe = runMockPasses(test->pipe, phv, defuse, uses);
    ASSERT_TRUE(pipe);

    auto* mocha = new CollectMochaCandidates(phv, uses, red_info, nonMochaDark);
    auto* after_analysis = pipe->apply(*mocha);
    ASSERT_TRUE(after_analysis);

    CollectNonDarkUses  nonDarkUses(phv, red_info);
    auto* dark = new MarkDarkCandidates(phv, uses, nonDarkUses);
    after_analysis = after_analysis->apply(nonDarkUses);
    after_analysis = after_analysis->apply(*dark);
    ASSERT_TRUE(after_analysis);

    const PHV::Field* h1f1 = phv.field("ingress::h1.f1"_cs);
    const PHV::Field* h1f2 = phv.field("ingress::h1.f2"_cs);
    const PHV::Field* h1f3 = phv.field("ingress::h1.f3"_cs);
    const PHV::Field* h1f4 = phv.field("ingress::h1.f4"_cs);
    const PHV::Field* h1p0 = phv.field("ingress::h1.pad_0"_cs);
    const PHV::Field* m1f1 = phv.field("ingress::meta.f1"_cs);
    const PHV::Field* m1f2 = phv.field("ingress::meta.f2"_cs);
    const PHV::Field* m1f3 = phv.field("ingress::meta.f3"_cs);
    const PHV::Field* m1f4 = phv.field("ingress::meta.f4"_cs);

    ASSERT_TRUE(h1f1 && h1f2 && h1f3 && h1f4 && h1p0 && m1f1 && m1f2 && m1f3 && m1f4);

    // Check mocha candidates.
    EXPECT_EQ(h1f1->is_mocha_candidate(), true);
    EXPECT_EQ(h1f2->is_mocha_candidate(), true);
    EXPECT_EQ(h1f3->is_mocha_candidate(), true);
    EXPECT_EQ(h1f4->is_mocha_candidate(), true);
    EXPECT_EQ(h1p0->is_mocha_candidate(), true);
    EXPECT_EQ(m1f1->is_mocha_candidate(), true);
    EXPECT_EQ(m1f2->is_mocha_candidate(), true);
    EXPECT_EQ(m1f3->is_mocha_candidate(), true);
    EXPECT_EQ(m1f4->is_mocha_candidate(), true);

    // Check dark candidates.
    EXPECT_EQ(h1f1->is_dark_candidate(), false);
    EXPECT_EQ(h1f2->is_dark_candidate(), false);
    EXPECT_EQ(h1f3->is_dark_candidate(), false);
    EXPECT_EQ(h1f4->is_dark_candidate(), false);
    EXPECT_EQ(m1f1->is_dark_candidate(), false);
    EXPECT_EQ(m1f2->is_dark_candidate(), false);
    EXPECT_EQ(m1f3->is_dark_candidate(), true);
    EXPECT_EQ(m1f4->is_dark_candidate(), true);
}

}  // namespace P4::Test
