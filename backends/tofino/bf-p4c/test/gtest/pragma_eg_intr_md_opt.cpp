/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <optional>
#include <boost/algorithm/string/replace.hpp>
#include <boost/make_unique.hpp>

#include "gtest/gtest.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/test/gtest/bf_gtest_helpers.h"


namespace P4::Test {

namespace {

struct FindEgIntrMd : public Inspector {
    const IR::Type_Header* eg_intr_md = nullptr;

    bool preorder(const IR::Type_Header* type) override {
        if (type->name == "egress_intrinsic_metadata_t") {
            if (!eg_intr_md) eg_intr_md = type;
        }
        return false;
    }
};

std::unique_ptr<TestCode> createCode(const std::string& pragma, const std::string& option) {
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

    auto insertions = {ss.str(), TestCode::empty_state(),
                     TestCode::empty_appy(), TestCode::empty_appy()};
    auto options = {option};
    auto test_code = boost::make_unique<TestCode>(TestCode::Hdr::TofinoMin,
                                                TestCode::tofino_shell(),
                                                insertions, "", options);

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

const IR::Type_Header* extractEgIntrMdType(TestCode* code) {
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
