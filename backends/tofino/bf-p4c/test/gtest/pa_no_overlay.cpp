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
#include "bf-p4c/phv/analysis/mutex_overlay.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class PaNoOverlayPragmaTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createPaNoOverlayPragmaTestCase() {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        @pa_no_overlay("ingress", "h2.f1")
        header H1
        {
            bit<32> f1;
            bit<32> f2;
        }
        header H2
        {
            bit<32> f1;
            bit<32> f2;
        }
        struct Headers { H1 h1; H2 h2; H1 h3; }
        struct Metadata {
            bit<32> f1;
            bit<32> f2;
        }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                transition select(headers.h1.f1) {
                    0       : parse_h2;
                    1       : parse_h3;
                    default : accept;
                }
            }

            state parse_h2 {
                packet.extract(headers.h2);
                transition accept;
            }

            state parse_h3 {
                packet.extract(headers.h3);
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

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPassesPaNoOverlay(const IR::BFN::Pipe *pipe, PhvInfo &phv) {
    PHV::Pragmas *pragmas = new PHV::Pragmas(phv);
    PhvUse *uses = new PhvUse(phv);
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(phv),
        pragmas,
        uses,
        new MutexOverlay(phv, *pragmas, *uses),
    };
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(PaNoOverlayPragmaTest, P4_16) {
    auto test = createPaNoOverlayPragmaTestCase();
    ASSERT_TRUE(test);

    PhvInfo phv;

    runMockPassesPaNoOverlay(test->pipe, phv);

    EXPECT_EQ(
        phv.field_mutex()(phv.field("ingress::h2.f2"_cs)->id, phv.field("ingress::h3.f2"_cs)->id),
        true);

    EXPECT_EQ(
        phv.field_mutex()(phv.field("ingress::h2.f1"_cs)->id, phv.field("ingress::h3.f1"_cs)->id),
        false);
}

}  // namespace P4::Test
