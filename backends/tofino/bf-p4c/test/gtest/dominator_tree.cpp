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

#include "bf-p4c/phv/analysis/dominator_tree.h"

#include <initializer_list>
#include <optional>
#include <vector>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class DominatorTreeTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createDominatorTreeTestCase(const std::string &parserSource) {
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

TEST_F(DominatorTreeTest, BasicControlFlow) {
    auto test = createDominatorTreeTestCase(P4_SOURCE(P4Headers::NONE, R"(

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

    ordered_map<gress_t, FlowGraph> fg;
    auto *dom_tree = new BuildDominatorTree(fg);
    test->pipe->apply(*dom_tree);

    EXPECT_EQ(dom_tree->hasImmediateDominator(INGRESS, "SINK"_cs), "t10_0"_cs);
    EXPECT_EQ(dom_tree->hasImmediateDominator(EGRESS, "SINK"_cs), "t10_0"_cs);
    EXPECT_EQ(dom_tree->hasImmediateDominator(INGRESS, "t11_0"_cs), "t11_0"_cs);
    EXPECT_EQ(dom_tree->hasImmediateDominator(EGRESS, "t11_0"_cs), "t11_0"_cs);
    EXPECT_EQ(dom_tree->hasImmediateDominator(INGRESS, "t7_0"_cs), "t4_0"_cs);
    EXPECT_EQ(dom_tree->hasImmediateDominator(EGRESS, "t3_0"_cs), "t1_0"_cs);

    EXPECT_EQ(dom_tree->strictlyDominates("t1_0"_cs, "t2_0"_cs, INGRESS), true);
    EXPECT_EQ(dom_tree->strictlyDominates("t11_0"_cs, "t8_0"_cs, INGRESS), true);
    EXPECT_EQ(dom_tree->strictlyDominates("t1_0"_cs, "t1_0"_cs, INGRESS), false);
    EXPECT_EQ(dom_tree->strictlyDominates("t7_0"_cs, "t11_0"_cs, INGRESS), false);
    EXPECT_EQ(dom_tree->strictlyDominates("t1_0"_cs, "t10_0"_cs, INGRESS), false);

    EXPECT_EQ(dom_tree->isDominator("t2_0"_cs, INGRESS, "t11_0"_cs), true);
    EXPECT_EQ(dom_tree->isDominator("t2_0"_cs, INGRESS, "t1_0"_cs), true);
    EXPECT_EQ(dom_tree->isDominator("t2_0"_cs, INGRESS, "t7_0"_cs), false);
}
}  // namespace P4::Test
