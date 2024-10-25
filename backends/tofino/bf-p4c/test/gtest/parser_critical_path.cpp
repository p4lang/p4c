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

#include "bf-p4c/phv/analysis/parser_critical_path.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

// Changes related to inserting parser states might break
// these tests.

namespace P4::Test {

class ParserCriticalPathTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createParserCriticalPathTestCase(
    const std::string &parserSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1
        {
            bit<8> f1;
            bit<32> f2;
        }
        header H2
        {
            bit<64> f1;
        }
        struct Headers { H1 h1; H2 h2; }
        struct Metadata {
            bit<8> f1;
            H2 f2;
        }

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

// In this unit-test, we do not check those inserted states before start (states has $ in name)
// Checking them make this unit-test flaky in that anyone who changes the inserted
// states would have to change this unit-test accrodingly.
std::vector<std::pair<cstring, int>> transformToComparable(
    const std::vector<std::pair<const IR::BFN::ParserState *, int>> &path) {
    std::vector<std::pair<cstring, int>> dest(path.size());
    transform(path.begin(), path.end(), dest.begin(),
              [](const std::pair<const IR::BFN::ParserState *, int> &x) {
                  return std::make_pair(x.first->name, x.second);
              });
    dest.erase(
        std::remove_if(dest.begin(), dest.end(),
                       [](std::pair<cstring, int> v) { return v.first.find('$') != nullptr; }),
        dest.end());
    return dest;
}

}  // namespace

TEST_F(ParserCriticalPathTest, SinglePath) {
    auto test = createParserCriticalPathTestCase(P4_SOURCE(P4Headers::NONE, R"(
state start {
    packet.extract(headers.h1);
    transition accept;
}
)"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    CalcParserCriticalPath *run = new CalcParserCriticalPath(phv);
    test->pipe->apply(*run);

    std::vector<std::pair<cstring, int>> expectedIngressPath({{"ingress::start"_cs, 41}});
    std::vector<std::pair<cstring, int>> expectedEgressPath({{"egress::start"_cs, 41}});

    EXPECT_EQ(transformToComparable(run->get_ingress_result().path), expectedIngressPath);
    EXPECT_EQ(transformToComparable(run->get_egress_result().path), expectedEgressPath);
}

TEST_F(ParserCriticalPathTest, TwoPath) {
    auto test = createParserCriticalPathTestCase(P4_SOURCE(P4Headers::NONE, R"(
state start {
    packet.extract(headers.h1);
    transition select(headers.h1.f1) {
        5 : parseH2;
        6 : parseH2AndMeta;
        default: accept;
    }
}

state parseH2 {
    packet.extract(headers.h2);
    transition accept;
}

state parseH2AndMeta {
    packet.extract(headers.h2);
    meta.f1 = headers.h1.f1;
    transition accept;
}
)"));
    ASSERT_TRUE(test);
    // ingress and egress should be the same here
    PhvInfo phv;
    CalcParserCriticalPath *run = new CalcParserCriticalPath(phv);
    test->pipe->apply(*run);

    std::vector<std::pair<cstring, int>> expectedIngressPath(
        {{"ingress::start"_cs, 41}, {"ingress::parseH2AndMeta"_cs, 65 + 8}});
    std::vector<std::pair<cstring, int>> expectedEgressPath({
        {"egress::start"_cs, 41}, {"egress::parseH2"_cs, 65}  // Egress does not extract metadata
    });

    EXPECT_EQ(transformToComparable(run->get_ingress_result().path), expectedIngressPath);
    EXPECT_EQ(transformToComparable(run->get_egress_result().path), expectedEgressPath);
}

}  // namespace P4::Test
