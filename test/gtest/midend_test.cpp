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
#include "helper.h"
#include "lib/log.h"

#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "midend/convertEnums.h"

#include "p4/p4-parse.h"

using namespace P4;

class EnumOn32Bits : public ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum*) const override {
        return true;
    }
    unsigned enumSize (unsigned) const override {
        return 32;
    }
};

// test various way of using enum
TEST(midend, convertEnums_pass) {
    std::string program =
        "enum E { A, B, C, D };\n"
        "const bool a = E.A == E.B;\n"
        "extern C { C(E e); }\n"
        "control m() { C(E.A) ctr; apply{} }\n";
    const IR::P4Program* pgm = parse_string(program);
    ASSERT_NE(nullptr, pgm);

    // Example to enable logging in source
    //Log::addDebugSpec("convertEnums:0");
    ReferenceMap  refMap;
    TypeMap       typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {
        convertEnums
    };
    pgm = pgm->apply(passes);
    ASSERT_NE(nullptr, pgm);
}

// use enum before declaration should fail
TEST(midend, convertEnums_used_before_declare) {
    std::string program =
        "const bool a = E.A == E.B;\n"
        "enum E { A, B, C, D };\n";
    const IR::P4Program* pgm = parse_string(program);
    ASSERT_NE(nullptr, pgm);

    ReferenceMap  refMap;
    TypeMap       typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {
        convertEnums
    };
    pgm = pgm->apply(passes);
    // expected pgm == nullptr
    ASSERT_EQ(nullptr, pgm);
}

// use enumMap in convertEnums directly
TEST(midend, getEnumMapping) {
    std::string program =
        "enum E { A, B, C, D };\n"
        "const bool a = E.A == E.B;\n";
    const IR::P4Program* pgm = parse_string(program);
    ASSERT_NE(nullptr, pgm);

    ReferenceMap  refMap;
    TypeMap       typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    P4::ConvertEnums::EnumMapping enumMap;
    PassManager passes = {
        convertEnums
    };
    pgm = pgm->apply(passes);
    ASSERT_NE(nullptr, pgm);

    enumMap = convertEnums->getEnumMapping();
    for (auto a : enumMap) {
        LOG1(a.first << " " << a.second);
    }
}

