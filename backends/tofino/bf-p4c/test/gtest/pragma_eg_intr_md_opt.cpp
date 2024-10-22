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
#include <boost/make_unique.hpp>

#include "bf-p4c/test/gtest/bf_gtest_helpers.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"

namespace P4::Test {

namespace {

struct FindEgIntrMd : public Inspector {
    const IR::Type_Header *eg_intr_md = nullptr;

    bool preorder(const IR::Type_Header *type) override {
        if (type->name == "egress_intrinsic_metadata_t") {
            if (!eg_intr_md) eg_intr_md = type;
        }
        return false;
    }
};

std::unique_ptr<TestCode> createCode(const std::string &pragma, const std::string &option) {
    std::stringstream ss;
    ss << pragma << '\n';
    ss << R"(
        header H1
        {
            bit<8> f1;
        }
        struct headers_t { H1 h1; }
        struct local_metadata_t { bit<8> f1; }
    )";

    auto insertions = {ss.str(), TestCode::empty_state(), TestCode::empty_appy(),
                       TestCode::empty_appy()};
    auto options = {option};
    auto test_code = boost::make_unique<TestCode>(
        TestCode::Hdr::TofinoMin, TestCode::tofino_shell(), insertions, "", options);

    BFNOptionPragmaParser parser;
    P4::ApplyOptionsPragmas global_pragmas_pass(parser);
    bool success = false;

    success = test_code->apply_pass(global_pragmas_pass);
    if (!success) {
        return nullptr;
    }
    success = test_code->apply_pass(TestCode::Pass::FullFrontend);
    if (!success) {
        return nullptr;
    }
    success = test_code->apply_pass(TestCode::Pass::FullMidend);
    if (!success) {
        return nullptr;
    }
    return test_code;
}

const IR::Type_Header *extractEgIntrMdType(TestCode *code) {
    FindEgIntrMd finder;
    code->apply_pass(finder);
    return finder.eg_intr_md;
}

}  // namespace

TEST(EgIntrMdOpt, CompareCmdOptionWithPragma) {
    auto code = createCode("", "");
    auto code_with_pragma = createCode("@pragma egress_intrinsic_metadata_opt", "");
    auto code_with_cmd_opt = createCode("", "--egress-intrinsic-metadata-opt");
    ASSERT_TRUE(code && code_with_pragma && code_with_cmd_opt);

    auto type = extractEgIntrMdType(code.get());
    auto type_with_pragma = extractEgIntrMdType(code_with_pragma.get());
    auto type_with_cmd_opt = extractEgIntrMdType(code_with_cmd_opt.get());
    ASSERT_TRUE(type && type_with_pragma && type_with_cmd_opt);

    // The metadata structs should be equivalent, with both cmd option and pragma
    EXPECT_TRUE(type_with_pragma->equiv(*type_with_cmd_opt));

    // If the pragma (or cmd option) was used, the type should be smaller
    EXPECT_LT(type_with_pragma->width_bits(), type->width_bits());
}

}  // namespace P4::Test
