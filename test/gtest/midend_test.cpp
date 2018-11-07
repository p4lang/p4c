/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "helpers.h"
#include "lib/log.h"

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "midend/convertEnums.h"

using namespace P4;

namespace Test {

namespace {

class EnumOn32Bits : public ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum*) const override {
        return true;
    }
    unsigned enumSize(unsigned) const override {
        return 32;
    }
};

}  // namespace

class P4CMidend : public P4CTest { };

// test various way of using enum
TEST_F(P4CMidend, convertEnums_pass) {
    std::string program = P4_SOURCE(R"(
        enum E { A, B, C, D };
        const bool a = E.A == E.B;
        extern C { C(E e); }
        control m() { C(E.A) ctr; apply{} }
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    // Example to enable logging in source
    // Log::addDebugSpec("convertEnums:0");
    ReferenceMap  refMap;
    TypeMap       typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {
        convertEnums
    };
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && errorCount() == 0);
}

TEST_F(P4CMidend, convertEnums_used_before_declare) {
    std::string program = P4_SOURCE(R"(
        const bool a = E.A == E.B;
        enum E { A, B, C, D };
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(pgm && ::errorCount() == 0);

    ReferenceMap  refMap;
    TypeMap       typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {
        convertEnums
    };
    pgm = pgm->apply(passes);
    // use enum before declaration should fail
    ASSERT_GT(::errorCount(), 0U);
}

// use enumMap in convertEnums directly
TEST_F(P4CMidend, getEnumMapping) {
    std::string program = P4_SOURCE(R"(
        enum E { A, B, C, D };
        const bool a = E.A == E.B;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(pgm != nullptr);

    ReferenceMap  refMap;
    TypeMap       typeMap;
    P4::ConvertEnums::EnumMapping enumMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes_ = {
        convertEnums
    };
    auto result = pgm->apply(passes_);
    ASSERT_TRUE(result != nullptr && ::errorCount() == 0);

    enumMap = convertEnums->getEnumMapping();
    ASSERT_EQ(enumMap.size(), (unsigned long)1);
}

}  // namespace Test
