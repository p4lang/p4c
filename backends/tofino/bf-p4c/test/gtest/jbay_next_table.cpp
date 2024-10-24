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

#include "bf-p4c/mau/jbay_next_table.h"

#include <array>
#include <initializer_list>
#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

class GTestTablePlacement : public PassManager {
    FlowGraph fg;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        fg.clear();
        return rv;
    }

    class VerifyFlowGraphAndPragmas : public MauInspector {
        FlowGraph &fg;
        ordered_map<const IR::MAU::Table *, int> table_to_logical_position;
        ordered_map<int, const IR::MAU::Table *> logical_position_to_table;
        ordered_map<int, int> stage_ids;  // map from stages to ids

        profile_t init_apply(const IR::Node *node) override {
            auto rv = MauInspector::init_apply(node);
            table_to_logical_position.clear();
            logical_position_to_table.clear();
            stage_ids.clear();
            return rv;
        }

        bool preorder(const IR::MAU::Table *tbl) override {
            int stage = tbl->get_provided_stage();
            if (stage == -1) {
                ::error("%1%: A stage must be provided on table %2%", tbl->srcInfo,
                        tbl->externalName());
                return true;
            }

            int logical_id = stage_ids[stage]++;
            if (logical_id > Memories::LOGICAL_TABLES) {
                ::error("%1%: Too many tables in stage %2%, %3% exceeds limit", tbl->srcInfo, stage,
                        tbl->externalName());
            }

            int glob_logical_id = stage * Memories::LOGICAL_TABLES + logical_id;
            auto lp_pos = logical_position_to_table.find(glob_logical_id);
            if (lp_pos != logical_position_to_table.end()) {
                ::error("%1%: Both table %2% and %3% have the same stage and logical id",
                        tbl->srcInfo, lp_pos->second->externalName(), tbl->externalName());
                return true;
            }

            logical_position_to_table[glob_logical_id] = tbl;
            table_to_logical_position[tbl] = glob_logical_id;
            return true;
        }

        void end_apply() override {
            // FIXME -- this doesn't actually test anything -- what should it test?
            typename FlowGraph::Graph::edge_iterator edge_it, edges_end;
            for (boost::tie(edge_it, edges_end) = boost::edges(fg.g); edge_it != edges_end;
                 ++edge_it) {
                auto src_v = boost::source(*edge_it, fg.g);
                auto src = fg.get_vertex(src_v);
                auto dst_v = boost::target(*edge_it, fg.g);
                auto dst = fg.get_vertex(dst_v);

                if (table_to_logical_position.count(src) == 0 ||
                    table_to_logical_position.count(dst) == 0) {
                    continue;
                }

                /* int src_pos = */ table_to_logical_position.at(src);
                /* int dst_pos = */ table_to_logical_position.at(dst);
            }
        }

     public:
        explicit VerifyFlowGraphAndPragmas(FlowGraph &f) : fg(f) {}
    };

    class PlaceTablesFromPragmas : public MauModifier {
        ordered_map<int, int> stage_ids;  // map from stages to ids
        profile_t init_apply(const IR::Node *node) override {
            stage_ids.clear();
            return MauModifier::init_apply(node);
        }

        bool preorder(IR::MAU::Table *tbl) override {
            int stage = tbl->get_provided_stage();
            BUG_CHECK(stage >= 0, "Invalid stage on %1%", tbl->externalName());
            int logical_id = stage_ids[stage]++;

            tbl->stage_ = stage;
            tbl->logical_id = logical_id;
            tbl->resources = new TableResourceAlloc();
            return true;
        }
    };

 public:
    GTestTablePlacement() {
        addPasses(
            {new FindFlowGraph(fg), new VerifyFlowGraphAndPragmas(fg), new PlaceTablesFromPragmas});
    }
};

/**
 * Specifically for running GTests to verify and validate the long branch allocation
 * in a very simple way.  Every table in the program should be provided a logical id within
 * a stage and a stage itself
 */

namespace P4::Test {

class NextTablePropTest : public JBayBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createNextTableCase(const std::string &ig_source,
                                                      const std::string &eg_source = "apply {} ") {
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
%MAU1%
}

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h1); }
}

control my_egress(inout Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
%MAU2%
}

V1Switch(parse(), verifyChecksum(), mau(), my_egress(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%MAU1%", ig_source);
    boost::replace_first(source, "%MAU2%", eg_source);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino2"_cs;
    options.arch = "v1model"_cs;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, JbayNextTable *nt) {
    PassManager quick_backend = {new MultipleApply(BackendOptions()), new GTestTablePlacement, nt};
    return pipe->apply(quick_backend);
}

}  // namespace

/* This test checks that when two uses overlap on the same gress that they are allocated onto
 * separate tags. We have one use from 0 to 3 and another from 0 to 5. These should not get merged
 * or dumb tabled. */
TEST_F(NextTablePropTest, OnGressOverlapLB) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
     action noop() { }

     @stage(0)
     table t1 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(3)
     table t2 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(0)
     table t3 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(5)
     table t4 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

    apply {
        if (t1.apply().hit) {
            t2.apply();
        }
        if (t3.apply().hit) {
            t4.apply();
        }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), 2UL);
}

/* Checks that the tightest on-gress merge occurs correctly. One use ends on 3 and the other begins
 * on 3, which should be merged. */
TEST_F(NextTablePropTest, TightOnGressMerge) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
     action noop() { }

     @stage(0)
     table t1 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(3)
     table t2 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(3)
     table t3 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(6)
     table t4 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

    apply {
        if (t1.apply().hit) {
            t2.apply();
        }
        if (t3.apply().hit) {
            t4.apply();
        }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), 1U);  // Should be able to merge
}

/* Checks that no merge occurs when one use ends on the same stage that another begins when the uses
 * are on different gresses. */
TEST_F(NextTablePropTest, TightOffGressOverlap) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
     action noop() { }

     @stage(0)
     table t1 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(3)
     table t2 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

    apply {
        if (t1.apply().hit) {
            t2.apply();
        }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(3)
    table t3 {
        key = {
            headers.h1.f1: exact;
        }

        actions = {
            noop;
        }
        size = 512;
    }

    @stage(6)
    table t4 {
        key = {
            headers.h1.f1: exact;
        }

        actions = {
            noop;
        }
        size = 512;
    }
    apply {
        if (t3.apply().hit) {
            t4.apply();
        }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), 2U);  // Can't merge!
}

/* Checks that a merge occurs when one use ends one stage before another begins when the uses
 * are on different gresses. This is the tightest cross-gress merge possible. */
TEST_F(NextTablePropTest, TightOffGressMerge) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
     action noop() { }

     @stage(0)
     table t1 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

     @stage(3)
     table t2 {
         key = {
             headers.h1.f1: exact;
         }

         actions = {
             noop;
         }
        size = 512;
     }

    apply {
        if (t1.apply().hit) {
            t2.apply();
        }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(4)
    table t3 {
        key = {
            headers.h1.f1: exact;
        }

        actions = {
            noop;
        }
        size = 512;
    }

    @stage(7)
    table t4 {
        key = {
            headers.h1.f1: exact;
        }

        actions = {
            noop;
        }
        size = 512;
    }
    apply {
        if (t3.apply().hit) {
            t4.apply();
        }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), 1U);
}

/* Checks that tags are successfully merged when we require 9 tags. All of the tags are from stage 0
 * to 13. As such, it is a full overlap and 12 dumb tables need to be used to merge the tags. */
TEST_F(NextTablePropTest, OnGressFullOverlapDumbTables) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t17 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t18 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
      if (t17.apply().hit) {
        t18.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));  // Check reduction success
    ASSERT_EQ(nt->get_num_dts(), 12U);  // Every possible merge requires 12 tables
}

/* Same as last, but now we have an optimal merge choice. The LBUses between t15 and t16 spans stage
 * 4 to 10, while the use between t17 and t18 spans from stage 7 to 13. Since they're on the same
 * gress, requires only 3 dumb tables. */
TEST_F(NextTablePropTest, OnGressPartialOverlapDumbTables) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(4)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(10)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(7)
    table t17 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t18 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
      if (t17.apply().hit) {
        t18.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));  // Check reduction success
    ASSERT_EQ(nt->get_num_dts(), 3U);  // Merge final two tags with only 3 tables
}

/* Same as last, but now we have two partial overlaps. Best to select the one that only requires 1
 * dumb table. 0-6 and 5-13 tags can be merged with a single dumb table. */
TEST_F(NextTablePropTest, OnGressPartialOverlapDumbTables2) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(6)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(4)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(10)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(5)
    table t17 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t18 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
      if (t17.apply().hit) {
        t18.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));
    ASSERT_EQ(nt->get_num_dts(), 1U);
}

/* Same as OnGressFullOverlap, but now the 9th tag is on the other gress. Since these are fully
 * overlapping tags, the merge will require elimination of an entire long branch. */
TEST_F(NextTablePropTest, OffGressFullOverlapDumbTables) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(4)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(10)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(7)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(10)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));
    ASSERT_EQ(nt->get_num_dts(), 2U);  // Add two tags to the 7-10 use to eliminate it
}

/* Same as last, but we are no longer fully overlapping. This is a difficult corner case, as merging
 * across gresses with partial overlap requires one more table than with partial overlap on the same
 * gress. */
TEST_F(NextTablePropTest, OffGressPartialOverlapDumbTables) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(4)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(10)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(7)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));
    ASSERT_EQ(nt->get_num_dts(), 4U);  // Add 4 to either partial overlaps is sufficient
}

/* Assures that when partial overlaps are avaiable they are not always selected. Here, we have a tag
 * with two uses on ingress that both partially overlap a use from egress. Instead of merging these
 * tags, it is most optimal to merge the egress tag with a large 0-13 tag on ingress, which should
 * correctly be discovered. */
TEST_F(NextTablePropTest, OffGressPartialOverlapMultiUseDumbTables) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(4)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(7)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t17 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(3)
    table t18 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
      if (t17.apply().hit) {
        t18.apply();
      }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(2)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(6)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));
    ASSERT_EQ(nt->get_num_dts(), 3U);  // Need to add 3 tables from stages 3-5 to the tag on egress
}

/* The "big, bad" test. Cross gress merging is required but there are multiple uses that partially
 * overlap. This tests a number of cornerc cases, including assuring that the selection of the best
 * tags to merge handles multiple uses. */
TEST_F(NextTablePropTest, OffGressPartialOverlapMultiUseDumbTables2) {
    auto test = createNextTableCase(P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(0)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t3 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t4 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t5 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t6 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t7 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t8 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t9 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t10 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t11 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t12 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t13 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t14 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(7)
    table t15 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(13)
    table t16 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(0)
    table t17 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(3)
    table t18 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }


    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
      if (t3.apply().hit) {
        t4.apply();
      }
      if (t5.apply().hit) {
        t6.apply();
      }
      if (t7.apply().hit) {
        t8.apply();
      }
      if (t9.apply().hit) {
        t10.apply();
      }
      if (t11.apply().hit) {
        t12.apply();
      }
      if (t13.apply().hit) {
        t14.apply();
      }
      if (t15.apply().hit) {
        t16.apply();
      }
      if (t17.apply().hit) {
        t18.apply();
      }
    }
)"),
                                    P4_SOURCE(P4Headers::NONE, R"(
    action noop() { }

    @stage(2)
    table t1 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    @stage(8)
    table t2 {
      key = {
      headers.h1.f1: exact;
      }

        actions = {
                   noop;
      }
      size = 512;
    }

    apply {
      if (t1.apply().hit) {
        t2.apply();
      }
    }
)"));
    ASSERT_TRUE(test);
    JbayNextTable *nt = new JbayNextTable(true);
    test->pipe = runMockPasses(test->pipe, nt);
    ASSERT_EQ(nt->get_num_lbs(), size_t(Device::numLongBranchTags()));
    ASSERT_EQ(nt->get_num_dts(), 4U);  // Best option is to add 4 tables to deal with the
                                       // double-partial overlap.
}
}  // namespace P4::Test
