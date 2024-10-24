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

#include "bf-p4c/phv/pragma/pa_container_size.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class PaContainerSizePragmaTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createPaContainerSizePragmaTestCase(const std::string &pragmas) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        %PRAGMAS%
        header H1
        {
            bit<8> f1;
            bit<32> f2;
        }
        header H2
        {
            bit<40> f1;
            bit<16> f2;
        }
        struct Headers { H1 h1; H2 h2; }
        struct Metadata {
            bit<8> f1;
            H2 f2;
        }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
            packet.extract(headers.h1);
            packet.extract(headers.h2);
            transition accept;
            }
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

    boost::replace_first(source, "%PRAGMAS%", pragmas);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, PhvInfo &phv,
                                   PragmaContainerSize &pa_cs) {
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(phv), &pa_cs};
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(PaContainerSizePragmaTest, SliceRequirement) {
    auto test = createPaContainerSizePragmaTestCase(P4_SOURCE(P4Headers::NONE,
                                                              R"(
                         @pa_container_size("ingress", "h1.f1", 8)
                         @pa_container_size("ingress", "h1.f2", 8)
                         @pa_container_size("ingress", "h2.f1", 8, 8, 8, 16)
                         @pa_container_size("ingress", "h2.f2", 24)
                         )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    PragmaContainerSize pa_cs(phv);
    runMockPasses(test->pipe, phv, pa_cs);

    auto *h1_f1 = phv.field("ingress::h1.f1"_cs);
    auto *h1_f2 = phv.field("ingress::h1.f2"_cs);
    auto *h2_f1 = phv.field("ingress::h2.f1"_cs);

    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h1_f1)), PHV::Size::b8);
    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h1_f2, FromTo(0, 7))), PHV::Size::b8);
    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h1_f2, FromTo(8, 15))), PHV::Size::b8);

    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h2_f1, FromTo(0, 7))), PHV::Size::b8);
    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h2_f1, FromTo(8, 15))), PHV::Size::b8);
    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h2_f1, FromTo(16, 23))),
              PHV::Size::b8);
    EXPECT_EQ(*pa_cs.expected_container_size(PHV::FieldSlice(h2_f1, FromTo(24, 39))),
              PHV::Size::b16);
}

}  // namespace P4::Test
