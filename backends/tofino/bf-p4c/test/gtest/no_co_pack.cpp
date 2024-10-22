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
#include <type_traits>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

namespace NoCoPackTest {

std::optional<TofinoPipeTestCase> createActionTest() {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1 { bit<8> f1; bit<8> f2; bit<8> f3; bit<8> f4; }
        header H2 { bit<16> f1; bit<16> f2; bit<16> f3; bit<16> f4; }
        header H3 { bit<32> f1; bit<32> f2; bit<32> f3; bit<32> f4; }

        struct Headers { H1 h1; H2 h2; H3 h3; }
        struct Metadata { }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                 inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                transition select(headers.h1.f1) {
                    2 : parse_h2;
                    default: accept;
                }
            }

            state parse_h2 {
                packet.extract(headers.h2);
                transition select(headers.h2.f1) {
                    3 : parse_h3;
                    default: accept;
                }
            }

            state parse_h3 {
                packet.extract(headers.h3);
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }
        control ingress(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply{ } }

        control egress(inout Headers headers, inout Metadata meta,
                       inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
                packet.emit(headers.h1);
                packet.emit(headers.h2);
                packet.emit(headers.h3);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runInitialPassManager(const IR::BFN::Pipe *pipe, PhvInfo *phv) {
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(*phv),
    };

    return pipe->apply(quick_backend);
}

}  // namespace NoCoPackTest

class NoCoPack : public TofinoBackendTest {};

TEST_F(NoCoPack, Sanity) {
    auto test = NoCoPackTest::createActionTest();

    ASSERT_TRUE(test);
    PhvInfo phv;

    NoCoPackTest::runInitialPassManager(test->pipe, &phv);

    auto h1_f1 = phv.field("ingress::h1.f1"_cs);
    auto h1_f2 = phv.field("ingress::h1.f2"_cs);
    auto h1_f4 = phv.field("ingress::h1.f4"_cs);
    auto h2_f1 = phv.field("ingress::h2.f1"_cs);
    auto h2_f4 = phv.field("ingress::h2.f4"_cs);
    auto h3_f1 = phv.field("ingress::h3.f1"_cs);
    auto h3_f2 = phv.field("ingress::h3.f2"_cs);
    auto h3_f4 = phv.field("ingress::h3.f4"_cs);

    EXPECT_FALSE(phv.isDeparserNoPack(h1_f1, h1_f1));
    EXPECT_FALSE(phv.isDeparserNoPack(h1_f4, h1_f4));

    EXPECT_TRUE(phv.isDeparserNoPack(h1_f4, h2_f1));
    EXPECT_TRUE(phv.isDeparserNoPack(h2_f4, h3_f1));

    EXPECT_TRUE(phv.isDeparserNoPack(h1_f4, h2_f4));
    EXPECT_TRUE(phv.isDeparserNoPack(h2_f4, h3_f4));

    EXPECT_FALSE(phv.isDeparserNoPack(h1_f2, h1_f4));
    EXPECT_TRUE(phv.isDeparserNoPack(h3_f2, h1_f4));
}

}  // namespace P4::Test
