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

#include <set>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

namespace {

using ActionSeq = std::vector<const IR::MAU::Action *>;

// This visitor captures all possible sequences of actions that can be applied.
struct CollectCFGPaths : public ControlFlowVisitor, public Inspector {
    CollectCFGPaths() {
        joinFlows = true;
        visitDagOnce = false;
    }

    ControlFlowVisitor *clone() const override { return new CollectCFGPaths(*this); }

    bool preorder(const IR::MAU::Action *act) override {
        std::set<ActionSeq> extended_paths;
        for (const auto &path : visited_paths) {
            auto ex_path = path;
            ex_path.push_back(act);
            extended_paths.insert(ex_path);
        }
        visited_paths = extended_paths;
        return true;
    }

    bool filter_join_point(const IR::Node *n) override {
        return !n->is<IR::MAU::Table>() && !n->is<IR::MAU::TableSeq>();
    }

    void flow_merge(Visitor &v) override {
        CollectCFGPaths &o = dynamic_cast<CollectCFGPaths &>(v);
        for (const ActionSeq &tbl : o.visited_paths) {
            visited_paths.insert(tbl);
        }
    }

    void flow_copy(ControlFlowVisitor &v) override {
        CollectCFGPaths &o = dynamic_cast<CollectCFGPaths &>(v);
        visited_paths = o.visited_paths;
    }

    std::set<ActionSeq> visited_paths = {ActionSeq()};
};

}  // namespace

TEST(MauControlFlowVisit, GatewayRunTableAndNext) {
    auto t1 = new IR::MAU::Table("t1"_cs, INGRESS);
    auto a1 = new IR::MAU::Action("a1"_cs);
    t1->actions["a1"_cs] = a1;

    auto t2 = new IR::MAU::Table("t2"_cs, INGRESS);
    auto a2 = new IR::MAU::Action("a2"_cs);
    t2->actions["a2"_cs] = a2;

    t1->gateway_rows.emplace_back(new IR::Constant(0), /* run table */ nullptr);
    t1->gateway_rows.emplace_back(/* miss */ nullptr, "$false");

    t1->next["$false"_cs] = new IR::MAU::TableSeq(t2);
    t1->match_key.push_back(new IR::MAU::TableKey(new IR::Constant(0), IR::ID()));

    CollectCFGPaths ccp;
    t1->apply(ccp);

    const std::set<ActionSeq> expected = {{a1}, {a2}};
    EXPECT_EQ(ccp.visited_paths, expected);
}

TEST(MauControlFlowVisit, GatewayRunTableAndFallthrough) {
    auto t1 = new IR::MAU::Table("t1"_cs, INGRESS);
    auto a1 = new IR::MAU::Action("a1"_cs);
    t1->actions["a1"_cs] = a1;

    auto t2 = new IR::MAU::Table("t2"_cs, INGRESS);
    auto a2 = new IR::MAU::Action("a2"_cs);
    t2->actions["a2"_cs] = a2;

    t1->gateway_rows.emplace_back(new IR::Constant(0), /* run table */ nullptr);
    t1->gateway_rows.emplace_back(/* miss */ nullptr, "$false");  // falls through

    t1->match_key.push_back(new IR::MAU::TableKey(new IR::Constant(0), IR::ID()));

    auto t_seq = new IR::MAU::TableSeq(t1, t2);

    CollectCFGPaths ccp;
    t_seq->apply(ccp);

    const std::set<ActionSeq> expected = {{a2}, {a1, a2}};
    EXPECT_EQ(ccp.visited_paths, expected);
}

}  // namespace P4::Test
