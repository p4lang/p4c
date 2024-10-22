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

#include "bf-p4c/common/elim_unused.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/phv/parde_phv_constraints.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

// Changes related to inserting parser states might break
// these tests.

namespace P4::Test {

class ParserConstantExtractTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createParserConstantExtractTestCase(
    const std::string &parserSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1
        {
            bit<16> f1;
            bit<16> f2;
            bit<16> f3;
            bit<16> f4;
        }
        struct Headers { H1 h1; H1 h2; H1 h3; H1 h4;
                         H1 h5; H1 h6; H1 h7; H1 h8;
                         H1 h9; H1 h10; }

        struct Metadata { }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
%PARSER_SOURCE%
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control mau(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply { }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%PARSER_SOURCE%", parserSource);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

}  // namespace

TEST_F(ParserConstantExtractTest, Test1) {
    auto test = createParserConstantExtractTestCase(P4_SOURCE(P4Headers::NONE, R"(

state start {
    packet.extract(headers.h1);
    packet.extract(headers.h2);
    transition select(headers.h1.f1) {
        0x0001 : parse1;
        0xFF80 : parse2;
        default: accept;
    }
}

state parse1 {
    packet.extract(headers.h3);
    packet.extract(headers.h4);
    transition select(headers.h3.f1) {
        0x0002 : parse3;
        0x0003 : parse4;
        default: accept;
    }
}

state parse2 {
    packet.extract(headers.h5);
    packet.extract(headers.h6);
    transition select(headers.h5.f1) {
        0x0001 : parse5;
        0xFF80 : parse6;
        default: accept;
    }
}

state parse3 {
    packet.extract(headers.h7);
    transition accept;
}

state parse4 {
    packet.extract(headers.h8);
    transition accept;
}

state parse5 {
    packet.extract(headers.h9);
    transition accept;
}

state parse6 {
    packet.extract(headers.h10);
    transition accept;
}

)"));
    ASSERT_TRUE(test);
    // ingress and egress should be the same here
    PhvInfo phv;

    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(phv),
                                 new TofinoParserConstantExtract(phv)};

    test->pipe->apply(quick_backend);

    const auto &unionFind = phv.getSameSetConstantExtraction();
    PHV::Field *v1 = phv.field("ingress::h1.$valid"_cs);
    PHV::Field *v2 = phv.field("ingress::h2.$valid"_cs);
    PHV::Field *v3 = phv.field("ingress::h3.$valid"_cs);
    PHV::Field *v4 = phv.field("ingress::h4.$valid"_cs);
    PHV::Field *v5 = phv.field("ingress::h5.$valid"_cs);
    PHV::Field *v6 = phv.field("ingress::h6.$valid"_cs);
    PHV::Field *v7 = phv.field("ingress::h7.$valid"_cs);
    PHV::Field *v8 = phv.field("ingress::h8.$valid"_cs);
    PHV::Field *v9 = phv.field("ingress::h9.$valid"_cs);
    PHV::Field *v10 = phv.field("ingress::h10.$valid"_cs);

    // Check that the field objects are correctly referenced.
    EXPECT_TRUE(v1);
    EXPECT_TRUE(v2);
    EXPECT_TRUE(v3);
    EXPECT_TRUE(v4);
    EXPECT_TRUE(v5);
    EXPECT_TRUE(v6);
    EXPECT_TRUE(v7);
    EXPECT_TRUE(v8);
    EXPECT_TRUE(v9);
    EXPECT_TRUE(v10);

    // Check that the appropriate fields have been marked by the bitvec in PhvInfo indicating that
    // they are extracted in the same parser state as at least one other POV bit.
    EXPECT_TRUE(phv.hasParserConstantExtract(v1));
    EXPECT_TRUE(phv.hasParserConstantExtract(v2));
    EXPECT_TRUE(phv.hasParserConstantExtract(v3));
    EXPECT_TRUE(phv.hasParserConstantExtract(v4));
    EXPECT_TRUE(phv.hasParserConstantExtract(v5));
    EXPECT_TRUE(phv.hasParserConstantExtract(v6));
    EXPECT_FALSE(phv.hasParserConstantExtract(v7));
    EXPECT_FALSE(phv.hasParserConstantExtract(v8));
    EXPECT_FALSE(phv.hasParserConstantExtract(v9));
    EXPECT_FALSE(phv.hasParserConstantExtract(v10));

    // Check set membership for each of the fields.
    const auto &s1 = unionFind.setOf(v1);
    const auto &s2 = unionFind.setOf(v2);
    EXPECT_EQ(1U, s1.count(v2));
    EXPECT_EQ(1U, s2.count(v1));
    EXPECT_EQ(0U, s1.count(v3));
    EXPECT_EQ(0U, s1.count(v5));
    EXPECT_EQ(0U, s1.count(v7));
    EXPECT_EQ(0U, s1.count(v9));
    EXPECT_EQ(0U, s2.count(v4));
    EXPECT_EQ(0U, s2.count(v6));
    EXPECT_EQ(0U, s2.count(v8));
    EXPECT_EQ(0U, s2.count(v10));
    const auto &s3 = unionFind.setOf(v3);
    const auto &s4 = unionFind.setOf(v4);
    const auto &s5 = unionFind.setOf(v5);
    const auto &s6 = unionFind.setOf(v6);
    const auto &s7 = unionFind.setOf(v7);
    const auto &s8 = unionFind.setOf(v8);
    const auto &s9 = unionFind.setOf(v9);
    const auto &s10 = unionFind.setOf(v10);
    EXPECT_EQ(1U, s3.count(v4));
    EXPECT_EQ(1U, s4.count(v3));
    EXPECT_EQ(1U, s5.count(v6));
    EXPECT_EQ(1U, s6.count(v5));
    EXPECT_EQ(0U, s7.count(v8));
    EXPECT_EQ(0U, s8.count(v7));
    EXPECT_EQ(0U, s9.count(v10));
    EXPECT_EQ(0U, s10.count(v9));
}

}  // namespace P4::Test
