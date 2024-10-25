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

#include "bf-p4c/mau/table_flow_graph.h"

#include <initializer_list>
#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class TableFlowGraphTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createTableFlowGraphTestCase(const std::string &parserSource) {
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

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

}  // namespace

/// Basic test case.
TEST_F(TableFlowGraphTest, BasicControlFlow) {
    auto test = createTableFlowGraphTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action setb1(bit<32> val) {
        headers.h2.b1 = val;
    }

    action setb2(bit<32> val) {
        headers.h2.b2 = val;
    }

    action setf1(bit<32> val) {
        headers.h2.f1 = val;
    }

    action setf2(bit<32> val) {
        headers.h2.f2 = val;
    }

    action setf3(bit<32> val) {
        headers.h2.f3 = val;
    }

    action setf4(bit<32> val) {
        headers.h2.f4 = val;
    }

    action setf5(bit<32> val) {
        headers.h2.f5 = val;
    }

    action setf6(bit<32> val) {
        headers.h2.f6 = val;
    }

    action setf7(bit<32> val) {
        headers.h2.f7 = val;
    }

    action setf8(bit<32> val) {
        headers.h2.f8 = val;
    }

    action setf9(bit<32> val) {
        headers.h2.f9 = val;
    }

    action setf10(bit<32> val) {
        headers.h2.f10 = val;
    }

    action setf12(bit<32> val) {
        headers.h2.f12 = val;
    }

    action noop() {

    }

    table t1 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            noop;
        }
        size = 8192;
    }

    table t2 {
        key = {
            headers.h2.b1 : exact;
        }
        actions  = {
            setb1;
            noop;
        }
        size = 8192;
    }

    table t7 {
        key = {
            headers.h2.b1 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t8 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t9 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t10 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t3 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf7;
        }
        size = 65536;
    }

    table t4 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf8;
        }
        size = 65536;
    }

    table t5 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf9;
        }
        size = 95536;
    }

    table t6 {
        key = {
            headers.h2.f1 : exact;
        }
        actions  ={
            setf10;
        }
        size = 95536;
    }

    table t11 {
        key = {
            headers.h2.f11 : exact;
        }
        actions = {
            noop;
            setf12;
        }
        size = 512;
    }

    table t12 {
        key = {
            headers.h2.f12 : exact;
        }
        actions = {
            noop;
        }
        size = 512;
    }

apply {
    if(t11.apply().hit) {
        t12.apply();
        if (t1.apply().hit) {
            t2.apply();
        }
        t3.apply();
        if (t4.apply().hit) {
            t5.apply();
        }
        else {
            t6.apply();
        }
        t7.apply();
    }

    t8.apply();
    t9.apply();
    t10.apply();
}
    )"));
    ASSERT_TRUE(test);

    FlowGraph fg;
    auto *find_fg_ingress = new FindFlowGraph(fg);

    test->pipe->thread[INGRESS].mau->apply(*find_fg_ingress);

    const IR::MAU::Table *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *t9, *t10, *t11, *t12;
    t1 = t2 = t3 = t4 = t5 = t6 = t7 = t8 = t9 = t10 = t11 = t12 = nullptr;
    typename FlowGraph::Graph::vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = boost::vertices(fg.g); v != v_end; ++v) {
        if (*v == fg.v_sink) continue;
        const IR::MAU::Table *found_table = fg.get_vertex(*v);
        if (found_table->externalName() == "mau.t1") {
            t1 = found_table;
        } else if (found_table->externalName() == "mau.t2") {
            t2 = found_table;
        } else if (found_table->externalName() == "mau.t3") {
            t3 = found_table;
        } else if (found_table->externalName() == "mau.t4") {
            t4 = found_table;
        } else if (found_table->externalName() == "mau.t5") {
            t5 = found_table;
        } else if (found_table->externalName() == "mau.t6") {
            t6 = found_table;
        } else if (found_table->externalName() == "mau.t7") {
            t7 = found_table;
        } else if (found_table->externalName() == "mau.t8") {
            t8 = found_table;
        } else if (found_table->externalName() == "mau.t9") {
            t9 = found_table;
        } else if (found_table->externalName() == "mau.t10") {
            t10 = found_table;
        } else if (found_table->externalName() == "mau.t11") {
            t11 = found_table;
        } else if (found_table->externalName() == "mau.t12") {
            t12 = found_table;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);
    EXPECT_NE(t4, nullptr);
    EXPECT_NE(t5, nullptr);
    EXPECT_NE(t6, nullptr);
    EXPECT_NE(t7, nullptr);
    EXPECT_NE(t8, nullptr);
    EXPECT_NE(t9, nullptr);
    EXPECT_NE(t10, nullptr);
    EXPECT_NE(t11, nullptr);
    EXPECT_NE(t12, nullptr);

    /* Build expected adjacency matrix:
     *
     *       t11
     *      /   \
     *    t12    |
     *     |     |
     *    t1     |
     *   / |     |
     *  t2 |     |
     *   \ |     |
     *    t3     |
     *     |     |
     *    t4     |
     *   /  \    |
     *  t5  t6   |
     *   \  /    /
     *    t7    /
     *      \  /
     *       t8
     *        |
     *       t9
     *        |
     *       t10
     *        |
     *     deparser
     */
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> expected_edges;
    expected_edges[t11].insert(t12);
    expected_edges[t12].insert(t1);
    expected_edges[t1].insert(t2);
    expected_edges[t1].insert(t3);
    expected_edges[t2].insert(t3);
    expected_edges[t3].insert(t4);
    expected_edges[t4].insert(t5);
    expected_edges[t4].insert(t6);
    expected_edges[t5].insert(t7);
    expected_edges[t6].insert(t7);
    expected_edges[t7].insert(t8);
    expected_edges[t11].insert(t8);
    expected_edges[t8].insert(t9);
    expected_edges[t9].insert(t10);
    expected_edges[t10].insert(nullptr);

    FlowGraph::Graph::edge_iterator edges, edges_end;
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> stored_edges;
    for (boost::tie(edges, edges_end) = boost::edges(fg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table *src = fg.get_vertex(boost::source(*edges, fg.g));
        const IR::MAU::Table *dst = fg.get_vertex(boost::target(*edges, fg.g));
        stored_edges[src].insert(dst);
    }

    // Check that expected_edges and stored_edges is the same. Do this by making sure all
    // stored_edges are expected, removing those edges from expected_edges, and making sure that
    // expected_edges is empty at the end.
    for (auto &kv : stored_edges) {
        auto &src = kv.first;
        auto &stored_dsts = kv.second;

        EXPECT_NE(expected_edges.count(src), UINT32_C(0));
        auto &expected_dsts = expected_edges.at(src);

        for (auto &dst : stored_dsts) {
            EXPECT_NE(expected_dsts.count(dst), UINT32_C(0));
            expected_dsts.erase(dst);
        }

        EXPECT_EQ(expected_dsts.size(), UINT32_C(0));
        expected_edges.erase(src);
    }
    EXPECT_EQ(expected_edges.size(), UINT32_C(0));

    // Test can_reach.
    EXPECT_EQ(fg.can_reach(t10, t11), false);
    EXPECT_EQ(fg.can_reach(t10, nullptr), true);
    EXPECT_EQ(fg.can_reach(nullptr, t10), false);
    EXPECT_EQ(fg.can_reach(t6, t5), false);
    EXPECT_EQ(fg.can_reach(t5, t6), false);
    EXPECT_EQ(fg.can_reach(t7, t7), false);
    EXPECT_EQ(fg.can_reach(t3, t7), true);

    // Test dominators.
    std::map<const IR::MAU::Table *, std::set<const IR::MAU::Table *>> expectedDominators;
    expectedDominators[t11] = {t11};
    expectedDominators[t12] = {t11, t12};
    expectedDominators[t1] = {t11, t12, t1};
    expectedDominators[t2] = {t11, t12, t1, t2};
    expectedDominators[t3] = {t11, t12, t1, t3};
    expectedDominators[t4] = {t11, t12, t1, t3, t4};
    expectedDominators[t5] = {t11, t12, t1, t3, t4, t5};
    expectedDominators[t6] = {t11, t12, t1, t3, t4, t6};
    expectedDominators[t7] = {t11, t12, t1, t3, t4, t7};
    expectedDominators[t8] = {t11, t8};
    expectedDominators[t9] = {t11, t8, t9};
    expectedDominators[t10] = {t11, t8, t9, t10};
    expectedDominators[nullptr] = {t11, t8, t9, t10};
    for (auto table_dominators : expectedDominators) {
        auto &table = table_dominators.first;
        auto &dominators = table_dominators.second;
        EXPECT_EQ(dominators, fg.get_dominators(table));
    }
}

/// Tests case where there cannot be a default-next table.
TEST_F(TableFlowGraphTest, NoDefaultNext) {
    auto test = createTableFlowGraphTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action setb1(bit<32> val) {
        headers.h2.b1 = val;
    }

    action setf7(bit<32> val) {
        headers.h2.f7 = val;
    }

    action setf8(bit<32> val) {
        headers.h2.f8 = val;
    }

    action noop() {

    }

    table t1 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            noop;
        }
        size = 8192;
    }

    table t2 {
        key = {
            headers.h2.b1 : exact;
        }
        actions  = {
            setb1;
            noop;
        }
        size = 8192;
    }

    table t3 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf7;
        }
        size = 65536;
    }

    table t4 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf8;
        }
        size = 65536;
    }

apply {
    if (t1.apply().hit) {
        t2.apply();
        t3.apply();
    } else {
        t2.apply();
    }
    t4.apply();
}
    )"));
    ASSERT_TRUE(test);

    FlowGraph fg;
    auto *find_fg_ingress = new FindFlowGraph(fg);

    // Apply MultipleApply to de-duplicate table nodes.
    auto mau =
        test->pipe->thread[INGRESS].mau->apply(*new MultipleApply(BackendOptions(), INGRESS, true));
    mau->apply(*find_fg_ingress);

    const IR::MAU::Table *t1, *t2, *t3, *t4;
    t1 = t2 = t3 = t4 = nullptr;
    typename FlowGraph::Graph::vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = boost::vertices(fg.g); v != v_end; ++v) {
        if (*v == fg.v_sink) continue;
        const IR::MAU::Table *found_table = fg.get_vertex(*v);
        if (found_table->externalName() == "mau.t1") {
            t1 = found_table;
        } else if (found_table->externalName() == "mau.t2") {
            t2 = found_table;
        } else if (found_table->externalName() == "mau.t3") {
            t3 = found_table;
        } else if (found_table->externalName() == "mau.t4") {
            t4 = found_table;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);
    EXPECT_NE(t4, nullptr);

    /* Build expected adjacency matrix:
     *
     *       t1 -- t2 -- t3 -- t4 -- deparser
     *               \________/
     */
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> expected_edges;
    expected_edges[t1].insert(t2);
    expected_edges[t2].insert(t3);
    expected_edges[t2].insert(t4);
    expected_edges[t3].insert(t4);
    expected_edges[t4].insert(nullptr);

    FlowGraph::Graph::edge_iterator edges, edges_end;
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> stored_edges;
    for (boost::tie(edges, edges_end) = boost::edges(fg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table *src = fg.get_vertex(boost::source(*edges, fg.g));
        const IR::MAU::Table *dst = fg.get_vertex(boost::target(*edges, fg.g));
        stored_edges[src].insert(dst);
    }

    // Check that expected_edges and stored_edges is the same. Do this by making sure all
    // stored_edges are expected, removing those edges from expected_edges, and making sure that
    // expected_edges is empty at the end.
    for (auto &kv : stored_edges) {
        auto &src = kv.first;
        auto &stored_dsts = kv.second;

        EXPECT_NE(expected_edges.count(src), UINT32_C(0));
        auto &expected_dsts = expected_edges.at(src);

        for (auto &dst : stored_dsts) {
            EXPECT_NE(expected_dsts.count(dst), UINT32_C(0));
            expected_dsts.erase(dst);
        }

        EXPECT_EQ(expected_dsts.size(), UINT32_C(0));
        expected_edges.erase(src);
    }
    EXPECT_EQ(expected_edges.size(), UINT32_C(0));

    // Test dominators.
    std::map<const IR::MAU::Table *, std::set<const IR::MAU::Table *>> expectedDominators;
    expectedDominators[t1] = {t1};
    expectedDominators[t2] = {t1, t2};
    expectedDominators[t3] = {t1, t2, t3};
    expectedDominators[t4] = {t1, t2, t4};
    expectedDominators[nullptr] = {t1, t2, t4};
    for (auto table_dominators : expectedDominators) {
        auto &table = table_dominators.first;
        auto &dominators = table_dominators.second;
        EXPECT_EQ(dominators, fg.get_dominators(table));
    }
}

/// Tests case where tables are not applied in consistent order on all branches.
TEST_F(TableFlowGraphTest, InconsistentApplyOrder) {
    auto test = createTableFlowGraphTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action setb1(bit<32> val) {
        headers.h2.b1 = val;
    }

    action setf7(bit<32> val) {
        headers.h2.f7 = val;
    }

    action noop() {

    }

    table t1 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            noop;
        }
        size = 8192;
    }

    table t2 {
        key = {
            headers.h2.b1 : exact;
        }
        actions  = {
            setb1;
            noop;
        }
        size = 8192;
    }

    table t3 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf7;
        }
        size = 65536;
    }

apply {
    if (t1.apply().hit) {
        t2.apply();
        t3.apply();
    } else {
        t3.apply();
        t2.apply();
    }
}
    )"));
    ASSERT_TRUE(test);

    FlowGraph fg;
    auto *find_fg_ingress = new FindFlowGraph(fg);

    // Apply MultipleApply to de-duplicate table nodes.
    auto mau =
        test->pipe->thread[INGRESS].mau->apply(*new MultipleApply(BackendOptions(), INGRESS, true));
    mau->apply(*find_fg_ingress);

    const IR::MAU::Table *t1, *t2, *t3;
    t1 = t2 = t3 = nullptr;
    typename FlowGraph::Graph::vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = boost::vertices(fg.g); v != v_end; ++v) {
        if (*v == fg.v_sink) continue;
        const IR::MAU::Table *found_table = fg.get_vertex(*v);
        if (found_table->externalName() == "mau.t1") {
            t1 = found_table;
        } else if (found_table->externalName() == "mau.t2") {
            t2 = found_table;
        } else if (found_table->externalName() == "mau.t3") {
            t3 = found_table;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);

    /* Build expected adjacency matrix:
     *
     *               +-----+
     *               V     |
     *       t1 --> t2 --> t3 --> deparser
     *        |      |     A         A
     *        +------------+         |
     *               |               |
     *               +---------------+
     */
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> expected_edges;
    expected_edges[t1].insert(t2);
    expected_edges[t2].insert(t3);
    expected_edges[t3].insert(nullptr);
    expected_edges[t1].insert(t3);
    expected_edges[t3].insert(t2);
    expected_edges[t2].insert(nullptr);

    FlowGraph::Graph::edge_iterator edges, edges_end;
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> stored_edges;
    for (boost::tie(edges, edges_end) = boost::edges(fg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table *src = fg.get_vertex(boost::source(*edges, fg.g));
        const IR::MAU::Table *dst = fg.get_vertex(boost::target(*edges, fg.g));
        stored_edges[src].insert(dst);
    }

    // Check that expected_edges and stored_edges is the same. Do this by making sure all
    // stored_edges are expected, removing those edges from expected_edges, and making sure that
    // expected_edges is empty at the end.
    for (auto &kv : stored_edges) {
        auto &src = kv.first;
        auto &stored_dsts = kv.second;

        EXPECT_NE(expected_edges.count(src), UINT32_C(0));
        auto &expected_dsts = expected_edges.at(src);

        for (auto &dst : stored_dsts) {
            EXPECT_NE(expected_dsts.count(dst), UINT32_C(0));
            expected_dsts.erase(dst);
        }

        EXPECT_EQ(expected_dsts.size(), UINT32_C(0));
        expected_edges.erase(src);
    }
    EXPECT_EQ(expected_edges.size(), UINT32_C(0));

    // Test can_reach.
    EXPECT_EQ(fg.can_reach(t2, t2), true);
    EXPECT_EQ(fg.can_reach(t2, t3), true);
    EXPECT_EQ(fg.can_reach(t3, t2), true);
    EXPECT_EQ(fg.can_reach(t3, t3), true);

    // Test dominators.
    std::map<const IR::MAU::Table *, std::set<const IR::MAU::Table *>> expectedDominators;
    expectedDominators[t1] = {t1};
    expectedDominators[t2] = {t1, t2};
    expectedDominators[t3] = {t1, t3};
    expectedDominators[nullptr] = {t1};
    for (auto table_dominators : expectedDominators) {
        auto &table = table_dominators.first;
        auto &dominators = table_dominators.second;
        EXPECT_EQ(dominators, fg.get_dominators(table));
    }
}

/// Tests case where tables are applied multiple times.
TEST_F(TableFlowGraphTest, MultipleApplies) {
    auto test = createTableFlowGraphTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action setb1(bit<32> val) {
        headers.h2.b1 = val;
    }

    action setb2(bit<32> val) {
        headers.h2.b2 = val;
    }

    action setf1(bit<32> val) {
        headers.h2.f1 = val;
    }

    action setf2(bit<32> val) {
        headers.h2.f2 = val;
    }

    action setf3(bit<32> val) {
        headers.h2.f3 = val;
    }

    action setf4(bit<32> val) {
        headers.h2.f4 = val;
    }

    action setf5(bit<32> val) {
        headers.h2.f5 = val;
    }

    action setf6(bit<32> val) {
        headers.h2.f6 = val;
    }

    action setf7(bit<32> val) {
        headers.h2.f7 = val;
    }

    action setf8(bit<32> val) {
        headers.h2.f8 = val;
    }

    action setf9(bit<32> val) {
        headers.h2.f9 = val;
    }

    action setf10(bit<32> val) {
        headers.h2.f10 = val;
    }

    action setf12(bit<32> val) {
        headers.h2.f12 = val;
    }

    action noop() {

    }

    table t1 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            noop;
        }
        size = 8192;
    }

    table t2 {
        key = {
            headers.h2.b1 : exact;
        }
        actions  = {
            setb1;
            noop;
        }
        size = 8192;
    }

    table t7 {
        key = {
            headers.h2.b1 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t8 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t9 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t10 {
        key = {
            headers.h2.f2 : exact;
        }
        actions = {
            setb1;
        }
        size = 512;
    }

    table t3 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf7;
        }
        size = 65536;
    }

    table t4 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf8;
        }
        size = 65536;
    }

    table t5 {
        key = {
            headers.h2.f1 : exact;
        }
        actions = {
            setf9;
        }
        size = 95536;
    }

    table t6 {
        key = {
            headers.h2.f1 : exact;
        }
        actions  ={
            setf10;
        }
        size = 95536;
    }

    table t11 {
        key = {
            headers.h2.f11 : exact;
        }
        actions = {
            noop;
            setf12;
        }
        size = 512;
    }

    table t12 {
        key = {
            headers.h2.f12 : exact;
        }
        actions = {
            noop;
        }
        size = 512;
    }

apply {
    if(t11.apply().hit) {
        t12.apply();
        if (t1.apply().hit) {
            t2.apply();
        }
        t3.apply();
        if (t4.apply().hit) {
            t5.apply();
        }
        else {
            t6.apply();
        }
        t2.apply();
        t7.apply();
    }

    t8.apply();
    t9.apply();
    t10.apply();
    t6.apply();
}
    )"));
    ASSERT_TRUE(test);

    FlowGraph fg;
    auto *find_fg_ingress = new FindFlowGraph(fg);

    // Apply MultipleApply to de-duplicate table nodes.
    auto mau =
        test->pipe->thread[INGRESS].mau->apply(*new MultipleApply(BackendOptions(), INGRESS, true));
    mau->apply(*find_fg_ingress);

    const IR::MAU::Table *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *t9, *t10, *t11, *t12;
    t1 = t2 = t3 = t4 = t5 = t6 = t7 = t8 = t9 = t10 = t11 = t12 = nullptr;
    typename FlowGraph::Graph::vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = boost::vertices(fg.g); v != v_end; ++v) {
        if (*v == fg.v_sink) continue;
        const IR::MAU::Table *found_table = fg.get_vertex(*v);
        if (found_table->externalName() == "mau.t1") {
            t1 = found_table;
        } else if (found_table->externalName() == "mau.t2") {
            t2 = found_table;
        } else if (found_table->externalName() == "mau.t3") {
            t3 = found_table;
        } else if (found_table->externalName() == "mau.t4") {
            t4 = found_table;
        } else if (found_table->externalName() == "mau.t5") {
            t5 = found_table;
        } else if (found_table->externalName() == "mau.t6") {
            t6 = found_table;
        } else if (found_table->externalName() == "mau.t7") {
            t7 = found_table;
        } else if (found_table->externalName() == "mau.t8") {
            t8 = found_table;
        } else if (found_table->externalName() == "mau.t9") {
            t9 = found_table;
        } else if (found_table->externalName() == "mau.t10") {
            t10 = found_table;
        } else if (found_table->externalName() == "mau.t11") {
            t11 = found_table;
        } else if (found_table->externalName() == "mau.t12") {
            t12 = found_table;
        }
    }

    EXPECT_NE(t1, nullptr);
    EXPECT_NE(t2, nullptr);
    EXPECT_NE(t3, nullptr);
    EXPECT_NE(t4, nullptr);
    EXPECT_NE(t5, nullptr);
    EXPECT_NE(t6, nullptr);
    EXPECT_NE(t7, nullptr);
    EXPECT_NE(t8, nullptr);
    EXPECT_NE(t9, nullptr);
    EXPECT_NE(t10, nullptr);
    EXPECT_NE(t11, nullptr);
    EXPECT_NE(t12, nullptr);

    /* Build expected adjacency matrix:
     *
     *                  t11
     *                 /   \
     *               t12    \
     *                |      |
     *               t1      |
     *              / |      |
     *       +---> t2 |      |
     *       |    / \ |      |
     *       |   /   t3      |
     *       |  |     |      |
     *       |  |    t4      |
     *       |  |   /  \     |
     *       |  |  t5  t6 <- | -------+
     *       |  |  |  /  \  /         |
     *       +- | -+--    \/          |
     *           \        /\          |
     *             t7    /  \         |
     *               \  /   deparser  |
     *                t8              |
     *                 |              |
     *                t9              |
     *                 |              |
     *                t10 ------------+
     */
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> expected_edges;
    expected_edges[t11].insert(t12);
    expected_edges[t12].insert(t1);
    expected_edges[t1].insert(t2);
    expected_edges[t1].insert(t3);
    expected_edges[t2].insert(t3);
    expected_edges[t2].insert(t7);
    expected_edges[t3].insert(t4);
    expected_edges[t4].insert(t5);
    expected_edges[t4].insert(t6);
    expected_edges[t5].insert(t2);
    expected_edges[t6].insert(t2);
    expected_edges[t6].insert(nullptr);
    expected_edges[t7].insert(t8);
    expected_edges[t11].insert(t8);
    expected_edges[t8].insert(t9);
    expected_edges[t9].insert(t10);
    expected_edges[t10].insert(t6);

    FlowGraph::Graph::edge_iterator edges, edges_end;
    std::map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> stored_edges;
    for (boost::tie(edges, edges_end) = boost::edges(fg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table *src = fg.get_vertex(boost::source(*edges, fg.g));
        const IR::MAU::Table *dst = fg.get_vertex(boost::target(*edges, fg.g));
        stored_edges[src].insert(dst);
    }

    // Check that expected_edges and stored_edges is the same. Do this by making sure all
    // stored_edges are expected, removing those edges from expected_edges, and making sure that
    // expected_edges is empty at the end.
    for (auto &kv : stored_edges) {
        auto &src = kv.first;
        auto &stored_dsts = kv.second;

        EXPECT_NE(expected_edges.count(src), UINT32_C(0));
        auto &expected_dsts = expected_edges.at(src);

        for (auto &dst : stored_dsts) {
            EXPECT_NE(expected_dsts.count(dst), UINT32_C(0));
            expected_dsts.erase(dst);
        }

        EXPECT_EQ(expected_dsts.size(), UINT32_C(0));
        expected_edges.erase(src);
    }
    EXPECT_EQ(expected_edges.size(), UINT32_C(0));

    // Test can_reach.
    EXPECT_EQ(fg.can_reach(t10, t11), false);
    EXPECT_EQ(fg.can_reach(t10, nullptr), true);
    EXPECT_EQ(fg.can_reach(nullptr, t10), false);
    EXPECT_EQ(fg.can_reach(t6, t5), true);
    EXPECT_EQ(fg.can_reach(t5, t6), true);
    EXPECT_EQ(fg.can_reach(t7, t7), true);
    EXPECT_EQ(fg.can_reach(t3, t7), true);
    EXPECT_EQ(fg.can_reach(t2, t2), true);
    EXPECT_EQ(fg.can_reach(t3, t2), true);
    EXPECT_EQ(fg.can_reach(t7, t2), true);
    EXPECT_EQ(fg.can_reach(t6, t6), true);
    EXPECT_EQ(fg.can_reach(t9, t8), true);

    // Test dominators.
    std::map<const IR::MAU::Table *, std::set<const IR::MAU::Table *>> expectedDominators;
    expectedDominators[t11] = {t11};
    expectedDominators[t12] = {t11, t12};
    expectedDominators[t1] = {t11, t12, t1};
    expectedDominators[t2] = {t11, t2};
    expectedDominators[t3] = {t11, t3};
    expectedDominators[t4] = {t11, t3, t4};
    expectedDominators[t5] = {t11, t3, t4, t5};
    expectedDominators[t6] = {t11, t6};
    expectedDominators[t7] = {t11, t2, t7};
    expectedDominators[t8] = {t11, t8};
    expectedDominators[t9] = {t11, t8, t9};
    expectedDominators[t10] = {t11, t8, t9, t10};
    expectedDominators[nullptr] = {t11, t6};
    for (auto table_dominators : expectedDominators) {
        auto &table = table_dominators.first;
        auto &dominators = table_dominators.second;
        EXPECT_EQ(dominators, fg.get_dominators(table));
    }
}
}  // namespace P4::Test
