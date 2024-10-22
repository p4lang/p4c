/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/mau/dynamic_dep_metrics.h"

#include <array>
#include <initializer_list>
#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class DynamicDepTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createDynamicDepMetricsCase(const std::string &ingressSource,
                                                              const std::string &egressSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
header H1
{
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
    bit<8> f6;
}

header H2
{
    bit<32> b1;
    bit<32> b2;
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<32> f6;
    bit<32> f7;
    bit<32> f8;
    bit<32> f9;
    bit<32> f10;
    bit<32> f11;
    bit<32> f12;
}

struct Headers { H1 h1; H2 h2;}
struct Metadata { }

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
%INGRESS_MAU%
}

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h1); }
}

control my_egress(inout Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
%EGRESS_MAU%
}

V1Switch(parse(), verifyChecksum(), mau(), my_egress(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS_MAU%", ingressSource);
    boost::replace_first(source, "%EGRESS_MAU%", egressSource);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, PhvInfo &phv,
                                   CalculateNextTableProp &ntp, ControlPathwaysToTable &paths,
                                   DependencyGraph &dg) {
    auto options = new BFN_Options();  // dummy options used in Pass
    ReductionOrInfo red_info;
    PassManager quick_backend = {new MultipleApply(BackendOptions()),
                                 new CollectHeaderStackInfo,
                                 new CollectPhvInfo(phv),
                                 new InstructionSelection(*options, phv, red_info),
                                 new CollectPhvInfo(phv),
                                 &ntp,
                                 &paths,
                                 new FindDependencyGraph(phv, dg)};
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(DynamicDepTest, DownwardProp1) {
    auto test = createDynamicDepMetricsCase(P4_SOURCE(P4Headers::NONE, R"(
    action a1() {
        headers.h1.f1 = 0;
    }

    action noop() {} 

    @name(".t1")
    table t1 {
        key = { headers.h1.f1 : exact; }
        actions = { a1; }
        size = 512;
    }

    @name(".t2")
    table t2 {
        key = { headers.h1.f1 : exact; }
        actions = { noop; }
        size = 512;
    }

    @name(".t3")
    table t3 {
        key = { headers.h1.f1 : exact; }
        actions = { noop; }
        size = 512;
    }

    action a4() {
        headers.h1.f4 = 0;
    }

    @name(".t4")
    table t4 {
        key = { headers.h1.f4 : exact; }
        actions = { a4; }
        size = 512;
    }

    @name(".t5")
    table t5 {
        key = { headers.h1.f4 : exact; }
        actions = { a4; }
        size = 512;
    }

    @name(".t6")
    table t6 {
        key = { headers.h1.f4 : exact; }
        actions = { a4; }
        size = 512;
    }

    apply {
        if (t1.apply().hit) {
            t2.apply();
            t3.apply();
        }

        t4.apply();
        t5.apply();
        t6.apply();
    }

        )"),
                                            P4_SOURCE(P4Headers::NONE, R"(

    action noop() {} 

    @name(".e1")
    table e1 {
        key = { headers.h1.f1 : exact; }
        actions = { noop; }
        size = 512;
    }

    apply {
        e1.apply(); 
    }
        )"));

    ASSERT_TRUE(test);

    PhvInfo phv;
    CalculateNextTableProp ntp;
    ControlPathwaysToTable paths;
    DependencyGraph dg;

    test->pipe = runMockPasses(test->pipe, phv, ntp, paths, dg);
    DynamicDependencyMetrics ddm(ntp, paths, dg);

    ordered_set<const IR::MAU::Table *> placed_tables;

    const IR::MAU::Table *t1, *t2, *t3, *t4, *t5, *t6, *e1;
    t1 = t2 = t3 = t4 = t5 = t6 = e1 = nullptr;
    for (const auto &kv : dg.stage_info) {
        cstring table_name = kv.first->externalName() + ""_cs;
        if (table_name == ".t1") {
            t1 = kv.first;
        } else if (table_name == ".t2") {
            t2 = kv.first;
        } else if (table_name == ".t3") {
            t3 = kv.first;
        } else if (table_name == ".t4") {
            t4 = kv.first;
        } else if (table_name == ".t5") {
            t5 = kv.first;
        } else if (table_name == ".t6") {
            t6 = kv.first;
        } else if (table_name == ".e1") {
            e1 = kv.first;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);
    EXPECT_NE(t4, nullptr);
    EXPECT_NE(t5, nullptr);
    EXPECT_NE(t6, nullptr);
    EXPECT_NE(e1, nullptr);
    auto scores = ddm.get_downward_prop_score(t1, e1);
    EXPECT_EQ(scores.first, 1);
    EXPECT_EQ(scores.second, 0);

    scores = ddm.get_downward_prop_score(t1, t4);
    EXPECT_EQ(scores.first, 1);
    EXPECT_EQ(scores.second, 2);

    placed_tables.emplace(t1);
    ddm.update_placed_tables(
        [&placed_tables](const IR::MAU::Table *tbl) -> bool { return placed_tables.count(tbl); });

    scores = ddm.get_downward_prop_score(t2, e1);
    EXPECT_EQ(scores.first, 2);
    EXPECT_EQ(scores.second, 0);

    scores = ddm.get_downward_prop_score(t2, t3);
    EXPECT_EQ(scores.first, 0);
    EXPECT_EQ(scores.second, 0);
}

TEST_F(DynamicDepTest, CanPlaceCDS) {
    auto test = createDynamicDepMetricsCase(P4_SOURCE(P4Headers::NONE, R"(
    action a1(bit<8> p) {
        headers.h1.f1 = p;
    }

    action a2(bit<8> p) {
        headers.h1.f2 = p;
    }

    action noop() {} 

    @name(".t1")
    table t1 {
        key = { headers.h1.f1 : exact; }
        actions = { a1; }
        size = 512;
    }

    @name(".t2")
    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { a2; }
        size = 512;
    }

    @name(".t3")
    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { noop; }
        size = 512;
    }

    @name(".t3_sub1")
    table t3_sub1 {
        key = { headers.h1.f1 : exact; }
        actions = { noop; }
        size = 512;
    }

    @name(".t3_sub2")
    table t3_sub2 {
        key = { headers.h1.f5 : exact; }
        actions = { noop; }
        size = 512;
    }


    @name(".t4")
    table t4 {
        key = { headers.h1.f4 : exact; }
        actions = { noop; }
        size = 512;
    }

    @name(".t4_sub1")
    table t4_sub1 {
        key = { headers.h1.f2 : exact; }
        actions = { noop; }
        size = 512;
    }

    apply {
        t1.apply();
        t2.apply();
        if (t3.apply().hit) {
            t3_sub1.apply();
            t3_sub2.apply();
        }

        if (t4.apply().hit) {
            t4_sub1.apply();
        }
    }
        )"),
                                            P4_SOURCE(P4Headers::NONE, R"(apply { })"));

    ASSERT_TRUE(test);

    PhvInfo phv;
    CalculateNextTableProp ntp;
    ControlPathwaysToTable paths;
    DependencyGraph dg;

    test->pipe = runMockPasses(test->pipe, phv, ntp, paths, dg);
    DynamicDependencyMetrics ddm(ntp, paths, dg);

    ordered_set<const IR::MAU::Table *> placed_tables;

    const IR::MAU::Table *t1, *t2, *t3, *t3_sub1, *t3_sub2, *t4, *t4_sub1;
    t1 = t2 = t3_sub1 = t3_sub2 = t4 = t4_sub1 = nullptr;
    for (const auto &kv : dg.stage_info) {
        cstring table_name = kv.first->externalName() + ""_cs;
        if (table_name == ".t1") {
            t1 = kv.first;
        } else if (table_name == ".t2") {
            t2 = kv.first;
        } else if (table_name == ".t3") {
            t3 = kv.first;
        } else if (table_name == ".t3_sub1") {
            t3_sub1 = kv.first;
        } else if (table_name == ".t3_sub2") {
            t3_sub2 = kv.first;
        } else if (table_name == ".t4") {
            t4 = kv.first;
        } else if (table_name == ".t4_sub1") {
            t4_sub1 = kv.first;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);
    EXPECT_NE(t3_sub1, nullptr);
    EXPECT_NE(t3_sub2, nullptr);
    EXPECT_NE(t4, nullptr);
    EXPECT_NE(t4_sub1, nullptr);

    EXPECT_TRUE(ddm.can_place_cds_in_stage(t3, placed_tables));
    EXPECT_TRUE(ddm.can_place_cds_in_stage(t4, placed_tables));

    placed_tables.emplace(t1);
    EXPECT_FALSE(ddm.can_place_cds_in_stage(t3, placed_tables));
    EXPECT_EQ(ddm.placeable_cds_count(t3, placed_tables), 2);
    EXPECT_TRUE(ddm.can_place_cds_in_stage(t4, placed_tables));

    placed_tables.emplace(t2);
    EXPECT_FALSE(ddm.can_place_cds_in_stage(t4, placed_tables));
}

}  // namespace P4::Test
