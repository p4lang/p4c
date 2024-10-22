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

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
namespace P4::Test {

class StaticEntriesConstPropTest : public TofinoBackendTest {};

namespace {

#define FABRIC_PKT_TYPE_ETH (0x00)  /* L2VPN */
#define FABRIC_PKT_TYPE_IPV4 (0x01) /* IPV4, L3VPN IPV4*/
#define FABRIC_PKT_TYPE_IPV6 (0x03) /* IPV6, L3VPN IPV6*/
#define FABRIC_PKT_TYPE_IP (0x05)

std::optional<TofinoPipeTestCase> createStaticEntriesConstPropTestCase(
    const std::string &ingress_source) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header ipv4_t {
            bit<4>  version;
            bit<4>  ihl;
            bit<8>  diffserv;
            bit<16> totalLen;
            bit<16> identification;
        }

        struct metadata {
bit<2> a;
bit<1> b;
        }

typedef bit<4> switch_pkt_type_t;

const switch_pkt_type_t FABRIC_PKT_TYPE_ETH = 0x00; /* L2VPN */
const switch_pkt_type_t FABRIC_PKT_TYPE_IPV4 = 0x01; /* IPV4, L3VPN IPV4*/
const switch_pkt_type_t FABRIC_PKT_TYPE_IPV6 = 0x03; /* IPV6, L3VPN IPV6*/
const switch_pkt_type_t FABRIC_PKT_TYPE_IP = 0x05;


        struct headers { ipv4_t data; }


        parser parse(packet_in packet, out headers hdr, inout metadata meta,
                 inout standard_metadata_t sm) {
            state start {
                packet.extract(hdr.data);
                transition accept;
            }
        }

        control verifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
        control ingress(inout headers hdr, inout metadata meta,
                        inout standard_metadata_t sm) {
            bit<4> md_version = 0;

            action md_version_set(bit<4> input_md_version) {
                md_version = input_md_version;
            }

            table set_md_version {
                actions = {
                    md_version_set;
                }
                size = 2;
            }

            action fabric_unicast_eth() {
                hdr.data.version = md_version;
                hdr.data.ihl = 0;
                hdr.data.identification = 1;
            }

            action fabric_unicast_ip() {
                hdr.data.version = md_version;
                hdr.data.ihl = 0;
                hdr.data.identification = 2;
            }

            table fabric_rewrite {
                key = {
                    md_version : exact;
                    hdr.data.diffserv : exact;
                }

                actions = {
                    NoAction;
                    fabric_unicast_eth;
                    fabric_unicast_ip;
                }

                %INGRESS%

                size = 128;
            }

            apply {
                set_md_version.apply();
                fabric_rewrite.apply();
            }
        }

        control egress(inout headers hdr, inout metadata meta,
                       inout standard_metadata_t sm) {
            apply { } }

        control computeChecksum(inout headers hdr, inout metadata meta) {
            apply { } }

        control deparse(packet_out packet, in headers hdr) {
            apply {
                packet.emit(hdr.data);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
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

const IR::BFN::Pipe *runCustomPassManager(const IR::BFN::Pipe *pipe, const BFN_Options &option,
                                          PhvInfo *phv) {
    ReductionOrInfo red_info;
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(*phv),
                                 new InstructionSelection(option, *phv, red_info)};

    return pipe->apply(quick_backend);
}

TEST_F(StaticEntriesConstPropTest, TestUniqueConstEntry) {
    auto test = createStaticEntriesConstPropTestCase(P4_SOURCE(P4Headers::NONE, R"(
            const default_action = NoAction;

            const entries = {
                (FABRIC_PKT_TYPE_ETH, 0) : fabric_unicast_eth();
                (FABRIC_PKT_TYPE_IPV6, 0) : fabric_unicast_ip();
            }
    )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    auto options = new BFN_Options();
    auto *post_pm = runCustomPassManager(test->pipe, *options, &phv);
    auto ingress_tables = post_pm->thread[0].mau->tables;

    const IR::MAU::Table *table_to_check = nullptr;
    for (auto table : ingress_tables) {
        if (table->name == "fabric_rewrite_0") table_to_check = table;
    }

    auto actions = table_to_check->actions;

    // Check fabric_unicast_eth action.
    auto *action_eth = actions["fabric_unicast_eth"_cs];
    auto *instr_eth = action_eth->action[0];
    EXPECT_EQ(instr_eth->name, "set");
    EXPECT_EQ(instr_eth->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 should be const propagated to FABRIC_PKT_TYPE_ETH.
    EXPECT_NE(instr_eth->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_eth->operands[1]->to<IR::Constant>()->value, FABRIC_PKT_TYPE_ETH);

    // Check fabric_unicast_ip action.
    auto *action_ip = actions["fabric_unicast_ip"_cs];
    auto *instr_ip = action_ip->action[0];
    EXPECT_EQ(instr_ip->name, "set");
    EXPECT_EQ(instr_ip->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 should be const propagated to FABRIC_PKT_TYPE_IPV6.
    EXPECT_NE(instr_ip->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_ip->operands[1]->to<IR::Constant>()->value, FABRIC_PKT_TYPE_IPV6);
}

TEST_F(StaticEntriesConstPropTest, TestMultipleConstEntry) {
    auto test = createStaticEntriesConstPropTestCase(P4_SOURCE(P4Headers::NONE, R"(
            const default_action = NoAction;

            const entries = {
                (FABRIC_PKT_TYPE_ETH, 0) : fabric_unicast_eth();
                (FABRIC_PKT_TYPE_IPV6, 0) : fabric_unicast_ip();
                (FABRIC_PKT_TYPE_IPV4, 0) : fabric_unicast_ip();
                (FABRIC_PKT_TYPE_IP, 0) : fabric_unicast_ip();
            }
    )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    auto options = new BFN_Options();
    auto *post_pm = runCustomPassManager(test->pipe, *options, &phv);
    auto ingress_tables = post_pm->thread[0].mau->tables;

    const IR::MAU::Table *table_to_check = nullptr;
    for (auto table : ingress_tables) {
        if (table->name == "fabric_rewrite_0") table_to_check = table;
    }

    auto actions = table_to_check->actions;

    // Check fabric_unicast_eth action
    auto *action_eth = actions["fabric_unicast_eth"_cs];
    auto *instr_eth = action_eth->action[0];
    EXPECT_EQ(instr_eth->name, "set");
    EXPECT_EQ(instr_eth->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 should be const propagated to FABRIC_PKT_TYPE_ETH.
    EXPECT_NE(instr_eth->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_eth->operands[1]->to<IR::Constant>()->value, FABRIC_PKT_TYPE_ETH);

    // Check fabric_unicast_ip action
    auto *action_ip = actions["fabric_unicast_ip"_cs];
    auto *instr_ip = action_ip->action[0];
    EXPECT_EQ(instr_ip->name, "set");
    EXPECT_EQ(instr_ip->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 shouldn't be const propagated to any constant.
    EXPECT_EQ(instr_ip->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_ip->operands[1]->toString(), "ingress::md_version_0");
}

TEST_F(StaticEntriesConstPropTest, TestUniqueConstEntryIsDefaultAction) {
    auto test = createStaticEntriesConstPropTestCase(P4_SOURCE(P4Headers::NONE, R"(
            const default_action = fabric_unicast_eth;

            const entries = {
                (FABRIC_PKT_TYPE_ETH, 0) : fabric_unicast_eth();
                (FABRIC_PKT_TYPE_IPV6, 0) : fabric_unicast_ip();
            }
    )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    auto options = new BFN_Options();
    auto *post_pm = runCustomPassManager(test->pipe, *options, &phv);
    auto ingress_tables = post_pm->thread[0].mau->tables;

    const IR::MAU::Table *table_to_check = nullptr;
    for (auto table : ingress_tables) {
        if (table->name == "fabric_rewrite_0") table_to_check = table;
    }

    auto actions = table_to_check->actions;

    // Check fabric_unicast_eth action
    auto *action_eth = actions["fabric_unicast_eth"_cs];
    auto *instr_eth = action_eth->action[0];
    EXPECT_EQ(instr_eth->name, "set");
    EXPECT_EQ(instr_eth->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 shouldn't be const propagated to const as it's also a default action.
    EXPECT_EQ(instr_eth->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_eth->operands[1]->toString(), "ingress::md_version_0");

    // Check fabric_unicast_ip action
    auto *action_ip = actions["fabric_unicast_ip"_cs];
    auto *instr_ip = action_ip->action[0];
    EXPECT_EQ(instr_ip->name, "set");
    EXPECT_EQ(instr_ip->operands[0]->toString(), "ingress::hdr.data.version");
    // Operand 1 should be const propagated to FABRIC_PKT_TYPE_IPV6.
    EXPECT_NE(instr_ip->operands[1]->to<IR::Constant>(), nullptr);
    EXPECT_EQ(instr_ip->operands[1]->to<IR::Constant>()->value, FABRIC_PKT_TYPE_IPV6);
}

}  // namespace P4::Test
