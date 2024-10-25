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
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

namespace {

std::optional<TofinoPipeTestCase> createComputedChecksumTestCase(
    const std::string &computeChecksumSource, const std::string &deparserSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H { bit<8> field1; bit<16> field2; bit<16> checksum; bit<32> field3; }
        struct Headers { H h1; H h2; }
        struct Metadata { H h; }

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

        control computeChecksum(inout Headers headers, inout Metadata meta) {
            apply {
%COMPUTE_CHECKSUM_SOURCE%
            }
        }

        control deparse(packet_out packet, in Headers headers) {
            apply {
%DEPARSER_SOURCE%
            }
        }

        V1Switch(parse(), verifyChecksum(), mau(), mau(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%COMPUTE_CHECKSUM_SOURCE%", computeChecksumSource);
    boost::replace_first(source, "%DEPARSER_SOURCE%", deparserSource);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::create(source);
}

void checkComputedChecksum(const IR::BFN::Pipe *pipe, const std::vector<cstring> &expected,
                           gress_t gress = EGRESS) {
    auto actual = pipe->thread[gress].deparser->to<IR::BFN::Deparser>()->emits.clone();

    for (size_t i = 0; i < expected.size(); ++i) {
        if (i >= actual->size()) {
            ADD_FAILURE() << "#" << i << " Missing: " << expected[i] << std::endl;
            continue;
        }

        if (expected[i] != cstring::to_cstring(actual->at(i)))
            ADD_FAILURE() << "#" << i << " Expected: " << expected[i] << std::endl
                          << "#" << i << " Actual: " << cstring::to_cstring(actual->at(i))
                          << std::endl;
    }

    for (auto i = expected.size(); i < actual->size(); ++i)
        ADD_FAILURE() << "#" << i << " Unexpected: " << expected[i] << std::endl;
}

}  // namespace

class TofinoComputedChecksum : public TofinoBackendTest {};

TEST_F(TofinoComputedChecksum, SimpleWithIsValid) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit headers.h1.field2 if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
         "emit headers.h1.field3 if headers.h1.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, SimpleWithoutIsValid) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit headers.h1.field2 if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
         "emit headers.h1.field3 if headers.h1.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, DuplicateHeader) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h2.field1,
                        headers.h2.field3},
                        headers.h2.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
        packet.emit(headers.h2);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit headers.h1.field2 if headers.h1.$valid"_cs,
         "emit headers.h1.checksum if headers.h1.$valid"_cs,  // We didn't compute this one.
         "emit headers.h1.field3 if headers.h1.$valid"_cs,

         "emit headers.h2.field1 if headers.h2.$valid"_cs,
         "emit headers.h2.field2 if headers.h2.$valid"_cs,
         "emit checksum { headers.h2.field1, headers.h2.field3 } if headers.h2.$valid"_cs,
         "emit headers.h2.field3 if headers.h2.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, DuplicateHeader2) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h2.field1,
                        headers.h2.field3 },
                        headers.h2.checksum,
                        HashAlgorithm.csum16);
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
        packet.emit(headers.h2);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit headers.h1.field2 if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
         "emit headers.h1.field3 if headers.h1.$valid"_cs,

         "emit headers.h2.field1 if headers.h2.$valid"_cs,
         "emit headers.h2.field2 if headers.h2.$valid"_cs,
         "emit checksum { headers.h2.field1, headers.h2.field3 } if headers.h2.$valid"_cs,
         "emit headers.h2.field3 if headers.h2.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, NotEmitted) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h2.field1,
                        headers.h2.field3 },
                        headers.h2.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(test->pipe, {"emit headers.h1.field1 if headers.h1.$valid"_cs,
                                       "emit headers.h1.field2 if headers.h1.$valid"_cs,
                                       "emit headers.h1.checksum if headers.h1.$valid"_cs,
                                       "emit headers.h1.field3 if headers.h1.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, EmittedTwice) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {
            "emit headers.h1.field1 if headers.h1.$valid"_cs,
            "emit headers.h1.field2 if headers.h1.$valid"_cs,
            "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
            "emit headers.h1.field3 if headers.h1.$valid"_cs,

            "emit headers.h1.field1 if headers.h1.$valid"_cs,
            "emit headers.h1.field2 if headers.h1.$valid"_cs,
            "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
            "emit headers.h1.field3 if headers.h1.$valid"_cs,
        });
}

TEST_F(TofinoComputedChecksum, ChecksumFieldInChecksum) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.checksum,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit headers.h1.field2 if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field1, headers.h1.checksum, headers.h1.field3 } if headers.h1.$valid"_cs,  // NOLINT
         "emit headers.h1.field3 if headers.h1.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, MultipleChecksumsInOneHeader) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field2,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.field2,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    checkComputedChecksum(
        test->pipe,
        {"emit headers.h1.field1 if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field1, headers.h1.field3 } if headers.h1.$valid"_cs,
         "emit checksum { headers.h1.field2, headers.h1.field3 } if headers.h1.$valid"_cs,
         "emit headers.h1.field3 if headers.h1.$valid"_cs});
}

TEST_F(TofinoComputedChecksum, DISABLED_ErrorEmptyChecksum) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {},
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_FALSE(test);
}

TEST_F(TofinoComputedChecksum, ErrorDestFieldMismatch) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h2.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
}

TEST_F(TofinoComputedChecksum, ErrorSourceFieldMismatch) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h2.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
}

TEST_F(TofinoComputedChecksum, ErrorSourceFieldMismatchWithoutIsValid) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h2.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_TRUE(test);
}

TEST_F(TofinoComputedChecksum, ErrorDestFieldNot16Bit) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        (bit<8>) headers.h1.field1,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_FALSE(test);
}

TEST_F(TofinoComputedChecksum, DISABLED_ErrorUnexpectedStatement) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        headers.h1.checksum,
                        HashAlgorithm.csum16);
        headers.h1.field2 = headers.h1.checksum;
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_FALSE(test);
}

TEST_F(TofinoComputedChecksum, ErrorNoDestField) {
    auto test = createComputedChecksumTestCase(P4_SOURCE(P4Headers::NONE, R"(
        update_checksum(true, {
                        headers.h1.field1,
                        headers.h1.field3 },
                        ,
                        HashAlgorithm.csum16);
    )"),
                                               P4_SOURCE(P4Headers::NONE, R"(
        packet.emit(headers.h1);
    )"));

    ASSERT_FALSE(test);
}

}  // namespace P4::Test
