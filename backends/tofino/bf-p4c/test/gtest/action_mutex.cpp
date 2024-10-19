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

#include "bf-p4c/mau/action_mutex.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class ActionMutexTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createActionMutexTestCase(const std::string &mau,
                                                            const std::string *tables_p = nullptr) {
    auto tables = P4_SOURCE(P4Headers::NONE, R"(
    action noop() {}

    action set_f1(bit<8> f1) {
        headers.h1.f1 = f1;
    }

    action set_f2(bit<8> f2) {
        headers.h1.f2 = f2;
    }

    action set_f3(bit<8> f3) {
        headers.h1.f3 = f3;
    }

    action set_f4(bit<8> f4) {
        headers.h1.f4 = f4;
    }

    action set_f5(bit<8> f5) {
        headers.h1.f5 = f5;
    }

    table node_a {
        actions = { set_f1; noop; }
        key = { headers.h1.f1 : exact; }
        const default_action = noop;
        size = 1024;
    }

    table node_b {
        actions = { set_f1; set_f2; set_f3; set_f5; }
        key = { headers.h1.f1 : exact; }
        size = 1024;
    }

    table node_c {
        actions = { set_f2; }
        key = { headers.h1.f1 : exact; }
        size = 1024;
    }

    table node_d {
        actions = { set_f2; set_f3; }
        key = { headers.h1.f1 : exact; }
        size = 1024;
    }

    table node_e {
        actions = { set_f4; }
        key = { headers.h1.f1 : exact; }
        size = 1024;
    }

    table node_f {
        actions = { set_f5; }
        key = { headers.h1.f1 : exact; }
        size = 1024;
    }

)");

    if (tables_p) tables = *tables_p;

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

struct Headers { H1 h1; }
struct Metadata { }

parser parse(packet_in packet, out Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
    state start {
        packet.extract(headers.h1);
        transition accept;
    }
}

control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control igrs(inout Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
%TABLES%
    apply {
%INGRESS%
    }
}

control egrs(inout Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
%TABLES%
    apply {
%EGRESS%
    }
}

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h1); }
}

V1Switch(parse(), verifyChecksum(), igrs(), egrs(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS%", mau);
    boost::replace_first(source, "%EGRESS%", mau);
    boost::replace_all(source, "%TABLES%", tables);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}
}  // namespace

TEST_F(ActionMutexTest, Basic) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
        node_a.apply();
        if (headers.h1.f1 == 0) {
            node_b.apply();
        } else {
            switch (node_d.apply().action_run) {
                set_f2: {
                    if (headers.h1.f2 == 1) {
                        node_e.apply();
                    }
                }
                set_f3: {
                    if (node_f.apply().hit) {
                        node_c.apply();
                    }
                }
            }
        }
)"));
    ASSERT_TRUE(test);

    ActionMutuallyExclusive act_mutex;
    test->pipe->apply(act_mutex);

    auto &act = act_mutex.name_actions;

    // debugging
    // for (auto a : act) std::cerr << a.first << std::endl;

    // not mutex
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_b.igrs.set_f1"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_b.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_b.igrs.set_f3"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_b.igrs.set_f5"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_d.igrs.set_f3"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_d.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));

    // ingress, egress
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("egrs.node_a.egrs.set_f1"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("egrs.node_d.egrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("egrs.node_e.egrs.set_f4"_cs)));

    // inside a table
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_b.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_b.igrs.set_f3"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_b.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_d.igrs.set_f3"_cs)));

    // between if and different depths
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_d.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f5"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));

    // from switch action_run children
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_d.igrs.set_f3"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f3"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));

    // between switch action_run
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_e.igrs.set_f4"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_e.igrs.set_f4"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));

    // from hit to children
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_f.igrs.set_f5"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
}

TEST_F(ActionMutexTest, DefaultNext) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            node_a.apply();
            switch (node_b.apply().action_run) {
                set_f1 : { node_c.apply(); }
                set_f2 : { node_d.apply(); }
                default : { node_e.apply(); node_f.apply(); }
            } )"));

    ASSERT_TRUE(test);
    ActionMutuallyExclusive act_mutex;
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f5"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f5"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
}

TEST_F(ActionMutexTest, MissingDefault) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            node_a.apply();
            switch (node_b.apply().action_run) {
                set_f1 : { node_c.apply(); }
                set_f2 : { node_d.apply(); }
            }
            node_e.apply();
            node_f.apply(); )"));

    ASSERT_TRUE(test);
    ActionMutuallyExclusive act_mutex;
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;

    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f5"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
}

TEST_F(ActionMutexTest, ConstDefaultAction) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            if (!node_a.apply().hit) {
                node_c.apply();
            }
            node_b.apply();
            node_d.apply();
            node_e.apply();
            node_f.apply(); )"));

    ASSERT_TRUE(test);
    ActionMutuallyExclusive act_mutex;
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_a.igrs.noop"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_a.igrs.set_f1"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
}

TEST_F(ActionMutexTest, SingleDefaultPath) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            node_a.apply();
            switch (node_b.apply().action_run) {
                default : { node_c.apply(); }
            }
            node_d.apply();
            node_e.apply();
            node_f.apply(); )"));

    ASSERT_TRUE(test);
    ActionMutuallyExclusive act_mutex;
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f1"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f2"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f3"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_b.igrs.set_f5"_cs), act.at("igrs.node_c.igrs.set_f2"_cs)));
}

TEST_F(ActionMutexTest, NextTableProperties) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            node_a.apply();
            switch (node_b.apply().action_run) {
                set_f1 : { node_c.apply(); node_e.apply(); }
                set_f2 : { node_d.apply(); node_e.apply(); }
                default : { node_f.apply(); }
            }
        )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    ActionMutuallyExclusive act_mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_d.igrs.set_f3"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
}

/**
 * Two actions for the same chain
 */
TEST_F(ActionMutexTest, TwoActionsSameChain) {
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            node_a.apply();
            switch (node_b.apply().action_run) {
                set_f1 :
                set_f2 : { node_c.apply(); node_d.apply(); }
                set_f3 : { node_e.apply(); }
                default : { node_f.apply(); }
            }
        )"));
    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    ActionMutuallyExclusive act_mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_e.igrs.set_f4"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_d.igrs.set_f3"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_c.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f2"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_e.igrs.set_f4"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
}

/**
 * Added to work with the new IR limits determined necessary for the Tofino2 based operations
 */
TEST_F(ActionMutexTest, Tofino2NextTableTest) {
    auto tables = P4_SOURCE(P4Headers::NONE, R"(
    action set_f1(bit<8> f1) {
        headers.h1.f1 = f1;
    }

    action set_f2(bit<8> f2) {
        headers.h1.f2 = f2;
    }

    action set_f3(bit<8> f3) {
        headers.h1.f3 = f3;
    }

    action set_f4(bit<8> f4) {
        headers.h1.f4 = f4;
    }

    action set_f5(bit<8> f5) {
        headers.h1.f5 = f5;
    }

    table node_a {
        key = { headers.h1.f1 : exact; }
        actions = { set_f1; set_f4; }
        size = 1024;
    }

    table node_b {
        key = { headers.h1.f1 : exact; }
        actions = { set_f1; set_f2; set_f3; set_f4; }
        size = 1024;
    }

    table node_c {
        key = { headers.h1.f1 : exact; }
        actions = { set_f1; set_f2; set_f3; set_f4; }
        size = 1024;
    }

    table node_d {
        key = { headers.h1.f1 : exact; }
        actions = { set_f5; }
        size = 1024;
    }

    table node_e {
        key = { headers.h1.f1 : exact; }
        actions = { set_f5; }
        size = 1024;
    }

    table node_f {
        key = { headers.h1.f1 : exact; }
        actions = { set_f5; }
        size = 1024;
    }

    table node_g {
        key = { headers.h1.f1 : exact; }
        actions = { set_f5; }
        size = 1024;
    }
    )");
    auto test = createActionMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
            if (node_a.apply().hit) {
                switch (node_b.apply().action_run) {
                    set_f1 : { node_d.apply(); }
                    set_f2 : { node_e.apply(); }
                    default : { node_f.apply(); }
                }
            } else {
                switch (node_c.apply().action_run) {
                    set_f1 : { node_d.apply(); node_e.apply(); node_f.apply(); }
                    set_f2 : { node_d.apply(); node_e.apply(); }
                    default : { node_f.apply(); node_g.apply(); }
                }
            }
        )"),
                                          &tables);

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    ActionMutuallyExclusive act_mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe->apply(act_mutex);
    auto &act = act_mutex.name_actions;
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_d.igrs.set_f5"_cs), act.at("igrs.node_g.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_e.igrs.set_f5"_cs), act.at("igrs.node_g.igrs.set_f5"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_d.igrs.set_f5"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_FALSE(
        act_mutex(act.at("igrs.node_e.igrs.set_f5"_cs), act.at("igrs.node_f.igrs.set_f5"_cs)));
    EXPECT_TRUE(
        act_mutex(act.at("igrs.node_b.igrs.set_f4"_cs), act.at("igrs.node_c.igrs.set_f4"_cs)));
}

}  // namespace P4::Test
