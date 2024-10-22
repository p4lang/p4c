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

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class TofinoFieldAlignment : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createFieldAlignmentTestCase(const std::string &headerSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H {
%HEADER_SOURCE%
        }
        struct Headers { H h; }
        struct Metadata { }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h);
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control mau(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
                packet.emit(headers.h);
            }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%HEADER_SOURCE%", headerSource);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::create(source);
}

/// A map from field names to expected alignment values. The expected alignment
/// is optional; if it's std::nullopt, then the field's alignment value must also
/// be std::nullopt - in other words, no particular alignment constraint should
/// have been inferred.
using ExpectedAlignmentMap = std::map<cstring, std::optional<unsigned>>;

/// Given a Tofino program, infer alignments for its fields and check that they
/// agree with the alignments we expect.
void checkFieldAlignment(const IR::BFN::Pipe *pipe, const ExpectedAlignmentMap &expected) {
    PhvInfo phv;
    PassManager computeAlignment = {new CollectHeaderStackInfo, new CollectPhvInfo(phv)};
    pipe->apply(computeAlignment);

    for (auto &item : expected) {
        auto fieldName = item.first;
        auto expectedAlignment = item.second;
        SCOPED_TRACE(fieldName);

        auto *fieldInfo = phv.field(fieldName);
        ASSERT_TRUE(fieldInfo != nullptr);

        if (!expectedAlignment) {
            EXPECT_TRUE(!fieldInfo->alignment);
            continue;
        }

        ASSERT_TRUE(fieldInfo->alignment);
        EXPECT_EQ(*expectedAlignment, fieldInfo->alignment->align);
    }
}

}  // namespace

TEST_F(TofinoFieldAlignment, ByteAlignedFields) {
    auto test = createFieldAlignmentTestCase(P4_SOURCE(P4Headers::NONE, R"(
        bit<8> field1;
        bit<16> field2;
        bit<32> field3;
        bit<64> field4;
        bit<8> field5;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    checkFieldAlignment(test->pipe, {
                                        {"h.field1"_cs, 0},
                                        {"h.field2"_cs, 0},
                                        {"h.field3"_cs, 0},
                                        {"h.field4"_cs, 0},
                                        {"h.field5"_cs, 0},
                                    });
}

TEST_F(TofinoFieldAlignment, SmallUnalignedFields) {
    auto test = createFieldAlignmentTestCase(P4_SOURCE(P4Headers::NONE, R"(
        bit<1> field1;
        bit<3> field2;
        bit<4> field3;
        bit<5> field4;
        bit<5> field5;
        bit<6> field6;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    checkFieldAlignment(test->pipe, {
                                        {"h.field1"_cs, 7},
                                        {"h.field2"_cs, 4},
                                        {"h.field3"_cs, 0},
                                        {"h.field4"_cs, 3},
                                        {"h.field5"_cs, 6},
                                        {"h.field6"_cs, 0},
                                    });
}

TEST_F(TofinoFieldAlignment, LargeUnalignedFields) {
    auto test = createFieldAlignmentTestCase(P4_SOURCE(P4Headers::NONE, R"(
        bit<7> field1;
        bit<9> field2;
        bit<17> field3;
        bit<36> field4;
        bit<11> field5;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    checkFieldAlignment(test->pipe, {
                                        {"h.field1"_cs, 1},
                                        {"h.field2"_cs, 0},
                                        {"h.field3"_cs, 7},
                                        {"h.field4"_cs, 3},
                                        {"h.field5"_cs, 0},
                                    });
}

TEST_F(TofinoFieldAlignment, NonPardeFieldsDoNotForceAlignment) {
    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    auto test = TofinoPipeTestCase::create(P4_SOURCE(P4Headers::V1MODEL, R"(
        header H {
            bit<8> field;
        }
        struct Headers { H usedInParser; H usedOnlyInMAU; H unused; }
        struct Metadata { bit<8> metadataField; }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
                meta.metadataField = 1;
                packet.extract(headers.usedInParser);
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control mau(inout Headers headers, inout Metadata meta,
                    inout standard_metadata_t sm) {
            apply {
                headers.usedOnlyInMAU.field = 1;
                headers.usedInParser.field = headers.usedOnlyInMAU.field;
            }
        }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
                packet.emit(headers.usedInParser);
            }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    checkFieldAlignment(test->pipe, {
                                        {"usedInParser.field"_cs, 0},
                                        {"usedOnlyInMAU.field"_cs, std::nullopt},
                                        {"meta.metadataField"_cs, std::nullopt},
                                    });
}

}  // namespace P4::Test
