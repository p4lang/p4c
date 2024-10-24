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

/*
 * This test tests whether a generated power.json file contains
 * names beginning with a dot, which is an issue for p4i.
 */

#include <fstream>
#include <iterator>
#include <regex>
#include <streambuf>
#include <string>

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"

namespace P4::Test {

TEST(PowerSchemaDotPrefix, Test1) {
    auto defs = R"(
        match_kind { exact, ternary, lpm }
        @noWarn("unused") action NoAction() {}
        header Hdr { bit<8> field1; bit<8> field2; bit<16> field3; bit<32> field4; }
        struct headers_t { Hdr h; }
        struct local_metadata_t {})";

    auto input = R"(
        table dummy_table_1 {
            key = {
                hdr.h.field4: exact;
            }
            actions = {
            }
        }
        @name("dummy_table_2") table dummy_table_2 {
            key = {
                hdr.h.field3: exact;
            }
            actions = {
            }
        }
        @name(".dummy_table_3") table dummy_table_3 {
            key = {
                hdr.h.field2: exact;
            }
            actions = {
            }
        }
        @name("ingress_control.dummy_table_4") table dummy_table_4 {
            key = {
                hdr.h.field1: exact;
            }
            actions = {
            }
        }
        apply {
            dummy_table_1.apply();
            dummy_table_2.apply();
            dummy_table_3.apply();
            dummy_table_4.apply();
        })";

    std::string output_dir = "PowerSchemaDotPrefix";

    auto blk = TestCode(TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),
                        {defs, TestCode::empty_state(), input, TestCode::empty_appy()}, "",
                        {"-o", output_dir, "--verbose"});
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullMidend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::ConverterToBackend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    std::string power_json = output_dir + "/pipeline/logs/power.json";

    std::ifstream power_json_stream(power_json);
    EXPECT_TRUE(power_json_stream);

    std::string line;
    std::regex name_regex("\"name\"\\s*:\\s*\"\\.[^\"]*\"");
    while (std::getline(power_json_stream, line)) {
        auto name_begin = std::sregex_iterator(line.begin(), line.end(), name_regex);
        auto name_end = std::sregex_iterator();
        EXPECT_TRUE(std::distance(name_begin, name_end) == 0)
            << "Found a dot at the beginning of the name: " << line;
    }
}

}  // namespace P4::Test
