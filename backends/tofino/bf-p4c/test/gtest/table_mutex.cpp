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

#include "bf-p4c/mau/table_mutex.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
namespace P4::Test {

class TableMutexTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createTableMutexTestCase(const std::string &ingress_source) {
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
%INGRESS%
}

control egrs(inout Headers headers, inout Metadata meta,
    inout standard_metadata_t sm) {
    apply {
    }
}

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h1); }
}

V1Switch(parse(), verifyChecksum(), igrs(), egrs(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS%", ingress_source);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;
    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

}  // namespace

TEST_F(TableMutexTest, BasicMutex) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; }
        size = 512;
    }

    apply {
        switch (t1.apply().action_run) {
            nop : { t2.apply(); }
            setf1 : { t3.apply(); }
        }
    }

    )"));
    ASSERT_TRUE(test);
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
}

TEST_F(TableMutexTest, TwoActionsSameNextTable) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(

    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; setf2; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; }
        size = 512;
    }


    apply {
        switch (t1.apply().action_run) {
            nop : {
                t2.apply();
            }
            setf1 : {
                t2.apply();
            }
            setf2 : {
                t3.apply();
            }
        }
    }

    )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
}

TEST_F(TableMutexTest, SharedNextTable) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; setf2; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; }
        size = 512;
    }

    table t4 {
        key = { headers.h1.f4 : exact; }
        actions = { nop; }
        size = 512;
    }


    apply {
        switch (t1.apply().action_run) {
            nop : {
                t2.apply();
            }
            setf1 : {
                t3.apply();
                t2.apply();
            }
            setf2 : {
                t4.apply();
            }
        }
    }

    )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t4"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t4"_cs)));
}

TEST_F(TableMutexTest, DifferingNextTableChains) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }
    action setf3() { headers.h1.f3 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; setf2; setf3; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; }
        size = 512;
    }

    table t4 {
        key = { headers.h1.f4 : exact; }
        actions = { nop; }
        size = 512;
    }

    table t5 {
        key = { headers.h1.f5 : exact; }
        actions = { nop; }
        size = 512;
    }


    apply {
        switch (t1.apply().action_run) {
            nop : {
                t2.apply();
            }
            setf1 : {
                t3.apply();
                t2.apply();
            }
            setf2 : {
                t4.apply();
            }
            setf3 : {
                t5.apply();
                t4.apply();
            }
        }
    }

    )"));
    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t4"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t5"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t5"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t5"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t4"_cs), names.at("igrs.t5"_cs)));
}

TEST_F(TableMutexTest, OneChainFromTwoActions) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }
    action setf3() { headers.h1.f3 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; setf2; setf3; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; }
        size = 512;
    }

    table t4 {
        key = { headers.h1.f4 : exact; }
        actions = { nop; }
        size = 512;
    }

    apply {
        switch (t1.apply().action_run) {
            nop: setf1 : {
                t2.apply();
            }
            setf2 : {
                t3.apply();
                t2.apply();
            }
            setf3 : {
                t4.apply();
            }
        }
    }

    )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t4"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t4"_cs)));
}

TEST_F(TableMutexTest, MultiLevelNextTableChain) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    action nop () {}
    action setf1(bit<8> p1) { headers.h1.f1 = p1; }
    action setf2() { headers.h1.f2 = headers.h1.f1; }
    action setf3() { headers.h1.f3 = headers.h1.f1; }

    table t1 {
        key = { headers.h1.f2 : exact; }
        actions = { nop; setf1; setf2; setf3; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f2 : exact; }
        actions = { setf1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f3 : exact; }
        actions = { setf2; setf3; }
        size = 512;
    }


    apply {
        switch (t1.apply().action_run) {
            nop: {
                t2.apply();
            }
            setf1 : {
                switch (t3.apply().action_run) {
                    setf3 : {
                        t2.apply();
                    }
                }
            }
        }
    }
    )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
}

TEST_F(TableMutexTest, Tofino2MultiApplyChain) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    action nop() {}
    action a1() { headers.h1.f1 = headers.h1.f2; }
    action a2() { headers.h1.f2 = headers.h1.f1; }
    action a3() { headers.h1.f3 = headers.h1.f1; }

    table t0 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; a2; a3; }
        size = 512;
    }

    table t1_0 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; a2; a3; }
        size = 512;
    }

    table t1_1 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; a2; a3; }
        size = 512;
    }

    table t2 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; }
        size = 512;
    }

    table t3 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a2; }
        size = 512;
    }

    table t4 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a3; }
        size = 512;
    }

    table t5 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; }
        size = 512;
    }

    table t6 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a2; }
        size = 512;
    }

    apply {
        if (t0.apply().hit) {
            switch(t1_0.apply().action_run) {
                a1 : { t2.apply(); }
                a2 : { t3.apply(); }
                a3 : { t4.apply(); } 
            }
        } else {
            switch(t1_1.apply().action_run) {
                a1 : { t2.apply(); t3.apply(); }
                a2 : { t3.apply(); t4.apply(); }
                a3 : { t4.apply(); t5.apply(); }
            }
        }
        t6.apply();
    }
    )"));

    ASSERT_TRUE(test);
    MultipleApply ma(BackendOptions());
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(ma);
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;

    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t5"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t5"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t1_0"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t6"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t5"_cs), names.at("igrs.t6"_cs)));
}

const IR::BFN::Pipe *runInitialPassManager(const IR::BFN::Pipe *pipe, const BFN_Options &option,
                                           PhvInfo *phv) {
    ReductionOrInfo red_info;
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(*phv),
                                 new InstructionSelection(option, *phv, red_info)};

    return pipe->apply(quick_backend);
}

TEST_F(TableMutexTest, IndirectAttachedActionAnalysis) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
    counter<bit<5>>(32, CounterType.packets_and_bytes) tcount;
    counter<bit<5>>(32, CounterType.packets_and_bytes) scount;
    action nop() {}
    action a1() { headers.h1.f1 = headers.h1.f2; }
    action a2() { tcount.count(1); }
    action a3() {tcount.count(2); }
    action a4() {scount.count(2); }
    action a5() { tcount.count(3); scount.count(3); }
   table t1 {
        key = { headers.h1.f1 : exact; }
        actions = { @defaultonly nop; a1; a2; }
        size = 512;
        const default_action = nop;
    }
    table t2 {
        key = { headers.h1.f3 : exact; }
        actions = { a1; a2; a3;}
        size = 512;
    }

    table t3 {
        key = { headers.h1.f4 : exact; }
        actions = { a1; a2; }
        size = 512;
    }

    table t4 {
        key = { headers.h1.f2 : exact; }
        actions = { a1; a4;}
        size = 512;
    }

    table t5 {
        key = { headers.h1.f5 : exact; }
        actions = { a1;}
        size = 512;
    }
 
   table t6 {
        key = { headers.h1.f1 : exact; }
        actions = { a1; a5;}
        size = 512;
    }

    table t7 {
        key = { headers.h1.f5 : exact; }
        actions = { a4;}
        size = 512;
    }

    table t8 {
        key = { headers.h1.f1 : exact; }
        actions = { a1;}
        size = 512;
    }
    apply {
        if (t5.apply().hit) {
            switch (t1.apply().action_run) {
                a1: { t2.apply();}
                default : {}
           }
        } else if (t8.apply().hit){
           switch (t3.apply().action_run) {
               a2 : {t4.apply();}
               default : {}
           }
       } else {
           switch (t6.apply().action_run) {
              a1: {t7.apply();}
              default : {}
           }
       }
    }
    )"));
    ASSERT_TRUE(test);
    IgnoreTableDeps ignore;
    TablesMutuallyExclusive mutex;
    ActionMutuallyExclusive action_mutex;
    PhvInfo phv;
    ReductionOrInfo red_info;
    SplitAttachedInfo att_info(phv);
    LayoutChoices lc(phv, red_info, att_info);
    SharedIndirectAttachedAnalysis sia(mutex, ignore, action_mutex, lc);
    auto options = new BFN_Options();
    auto *post_pm_pipe = runInitialPassManager(test->pipe, *options, &phv);
    post_pm_pipe = post_pm_pipe->apply(mutex);
    post_pm_pipe = post_pm_pipe->apply(action_mutex);
    post_pm_pipe = post_pm_pipe->apply(ignore);
    post_pm_pipe = post_pm_pipe->apply(sia);
    auto &names = mutex.name_to_tables;
    EXPECT_TRUE(sia.if_table_share_attach(names.at("igrs.t1"_cs), names.at("igrs.t2"_cs)));
    EXPECT_FALSE(sia.if_table_share_attach(names.at("igrs.t3"_cs), names.at("igrs.t4"_cs)));
    EXPECT_TRUE(sia.if_table_share_attach(names.at("igrs.t1"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(sia.if_table_share_attach(names.at("igrs.t6"_cs), names.at("igrs.t7"_cs)));
}

TEST_F(TableMutexTest, ExitTableMutex1) {
    auto test = createTableMutexTestCase(P4_SOURCE(P4Headers::NONE, R"(
     action nop() {}
     action a1() { headers.h1.f1 = headers.h1.f2; }
     action a2() { exit;}
     action a3() { headers.h1.f2 = headers.h1.f3;}
     action a4() { headers.h1.f1 = headers.h1.f3;}

     table t1 {
        key = { headers.h1.f5 : exact; }
        actions = { @defaultonly nop; a1; a3;}
        size = 512;
        const default_action = nop;
    }

    table t2 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1; a3;}
        size = 512;
    }

    table t3 {
        key = { headers.h1.f2 : exact;}
        actions = {a2;}
        size = 512;
        const default_action = a2; 
    }
    table t4 {
        key = { headers.h1.f3 : exact;}
        actions = {nop; a1; a3;}
        size = 512;
    }
    table t5 {
        key = { headers.h1.f4 : exact;}
        actions = {nop; a1; a3;}
        size = 512;
    }
    table t6 {
        key = { headers.h1.f3 : exact;}
        actions = {nop; a1;}
        size = 512;
    }
    table t7 {
        key = { headers.h1.f3 : exact;}
        actions = {a2;}
        size = 512;
        const default_action = a2;
    }
    table t8 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a1;}
        size = 512;
    }
    table t9 {
        key = { headers.h1.f1 : exact; }
        actions = { nop; a4;}
        size = 512;
    }

    apply {
        t9.apply();
        if (t1.apply().hit) {
            switch(t2.apply().action_run) {
                a1: {t5.apply();}
                a3: {t4.apply(); t3.apply();}
                default : {}
            }
        } else {
            switch(t6.apply().action_run) {
                a1: {t7.apply();}
                nop: {}
            }
        }
        t8.apply();
    }
    )"));
    ASSERT_TRUE(test);
    TablesMutuallyExclusive mutex;
    test->pipe = test->pipe->apply(mutex);
    auto &names = mutex.name_to_tables;
    EXPECT_FALSE(mutex(names.at("igrs.t2"_cs), names.at("igrs.t3"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t4"_cs), names.at("igrs.t8"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t3"_cs), names.at("igrs.t8"_cs)));
    EXPECT_TRUE(mutex(names.at("igrs.t7"_cs), names.at("igrs.t8"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t9"_cs), names.at("igrs.t4"_cs)));
    EXPECT_FALSE(mutex(names.at("igrs.t5"_cs), names.at("igrs.t8"_cs)));
}

}  // namespace P4::Test
