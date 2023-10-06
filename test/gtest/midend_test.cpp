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

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "midend/convertEnums.h"
#include "midend/replaceSelectRange.h"

using namespace P4;

namespace Test {

namespace {

class EnumOn32Bits : public ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum *) const override { return true; }
    unsigned enumSize(unsigned) const override { return 32; }
};

}  // namespace

class P4CMidend : public P4CTest {};

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
    ReferenceMap refMap;
    TypeMap typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {convertEnums};
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && errorCount() == 0);
}

TEST_F(P4CMidend, convertEnums_used_before_declare) {
    std::string program = P4_SOURCE(R"(
        const bool a = E.A == E.B;
        enum E { A, B, C, D };
    )");
    P4CContext::get().options().langVersion = CompilerOptions::FrontendVersion::P4_16;
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(pgm && ::errorCount() == 0);

    ReferenceMap refMap;
    TypeMap typeMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes = {convertEnums};
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

    ReferenceMap refMap;
    TypeMap typeMap;
    P4::ConvertEnums::EnumMapping enumMap;
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager passes_ = {convertEnums};
    auto result = pgm->apply(passes_);
    ASSERT_TRUE(result != nullptr && ::errorCount() == 0);

    enumMap = convertEnums->getEnumMapping();
    ASSERT_EQ(enumMap.size(), (unsigned long)1);
}

class CollectRangesAndMasks : public Inspector {
 public:
    std::vector<const IR::Range *> ranges;
    std::vector<const IR::Mask *> masks;

    bool preorder(const IR::Range *r) override {
        ranges.emplace_back(r);
        return true;
    }

    bool preorder(const IR::Mask *m) override {
        masks.emplace_back(m);
        return true;
    }
};

using Bound = std::pair<int, int>;
using Mask = std::pair<int, int>;

// note: the callback is used so that SCOPED_TRACE is able to add additional
// information visible even if the tests in the callback fail
template <typename T = uint16_t, typename ExtraTests>
static void testReplaceSelectRange(std::vector<Bound> ranges, ExtraTests extraTests) {
    static_assert(std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value,
                  "only uint16_t and int16_t can be used as template argument "
                  "of testReplaceSelectRange");
    std::stringstream code;
    code << R"(
        struct bs {
            )"
         << (std::is_signed<T>::value ? "int" : "bit") << R"(<16> x;
        }

        parser p(in bs b, out bool matches)
        {
            state start
            {
                transition select(b.x) {)";
    code << '\n';

    for (auto p : ranges) {
        code << std::string(20, ' ') << p.first << ".." << p.second << " : next;\n";
    }
    code << R"(                }
            }
            state next { }
        }
    )";
    auto codeStr = code.str();
    SCOPED_TRACE(codeStr);
    std::string program = P4_SOURCE(codeStr.c_str());
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(pgm != nullptr);

    ReferenceMap refMap;
    TypeMap typeMap;

    PassManager passes_ = {new P4::ResolveReferences(&refMap),
                           new P4::TypeInference(&refMap, &typeMap, false),
                           // properly set types for compound expressions
                           new P4::TypeChecking(&refMap, &typeMap, true),
                           new P4::ReplaceSelectRange(&refMap, &typeMap)};
    auto result = pgm->apply(passes_);
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(::errorCount(), 0u);

    CollectRangesAndMasks collect;
    result->apply(collect);
    ASSERT_FALSE(collect.masks.empty());

    std::vector<Mask> maskVals;
    std::stringstream masksStr("masks: ");
    for (auto mask : collect.masks) {
        auto left = mask->left->to<IR::Constant>();
        ASSERT_TRUE(left != nullptr);
        auto right = mask->right->to<IR::Constant>();
        ASSERT_TRUE(right != nullptr);
        maskVals.emplace_back(unsigned(left->value), unsigned(right->value));
        masksStr << std::hex << "0x" << left->value << " &&& 0x" << right->value << "; ";
    }
    SCOPED_TRACE(masksStr.str());

    int lo = std::numeric_limits<T>::min();
    int hi = std::numeric_limits<T>::max();
    for (int i = lo; i <= hi; ++i) {
        bool inRange = std::any_of(ranges.begin(), ranges.end(),
                                   [=](Bound b) { return b.first <= i && i <= b.second; });

        bool inMask = std::any_of(maskVals.begin(), maskVals.end(),
                                  [=](Mask m) { return (i & m.second) == (m.first & m.second); });
        ASSERT_EQ(inRange, inMask) << "value " << i;
    }

    ASSERT_EQ(collect.ranges.size(), 0u);

    extraTests(collect);
}

// test that 0..X ranges are replaced efficienlty by masks
TEST_F(P4CMidend, replaceSelectRangeZero1) {
    testReplaceSelectRange({{0, 255}}, [](CollectRangesAndMasks collect) {
        ASSERT_EQ(collect.masks.size(), 1u);  // 0 &&& 0xff00
    });
}

TEST_F(P4CMidend, replaceSelectRangeZero2) {
    testReplaceSelectRange({{0, 254}}, [](CollectRangesAndMasks collect) {
        /*  0x00 &&& 0xff80
            0x80 &&& 0xffc0
            0xc0 &&& 0xffe0
            0xe0 &&& 0xfff0
            0xf0 &&& 0xfff8
            0xf8 &&& 0xfffc
            0xfc &&& 0xfffe
            0xfe &&& 0xffff */
        ASSERT_EQ(collect.masks.size(), 8u);
    });
}

TEST_F(P4CMidend, replaceSelectRangeZero3) {
    testReplaceSelectRange({{0, 256}}, [](CollectRangesAndMasks collect) {
        ASSERT_EQ(collect.masks.size(), 2u);  // 0 &&& 0xff00 + 0x0100 &&& 0xffff
    });
}

TEST_F(P4CMidend, replaceSelectRangeZero4) {
    testReplaceSelectRange({{0, 63 | 256}}, [](CollectRangesAndMasks collect) {
        ASSERT_EQ(collect.masks.size(), 2u);  // 0 &&& 0xff00 + 0x0100 &&& 0xffc0
    });
}

TEST_F(P4CMidend, replaceSelectRangeZero5) {
    testReplaceSelectRange(
        {{0, 1235}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 5u); });
}

TEST_F(P4CMidend, replaceSelectRangeZero6) {
    testReplaceSelectRange(
        {{0, 65535}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 1u); });
}

TEST_F(P4CMidend, replaceSelectRangeZero7) {
    testReplaceSelectRange(
        {{0, 0}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 1u); });
}

TEST_F(P4CMidend, replaceSelectRangeZero8) {
    testReplaceSelectRange(
        {{0, 1}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 1u); });
}

TEST_F(P4CMidend, replaceSelectRange1) {
    testReplaceSelectRange(
        {{16, 255}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 4u); });
}

TEST_F(P4CMidend, replaceSelectRange2) {
    testReplaceSelectRange(
        {{20, 319}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 6u); });
}

TEST_F(P4CMidend, replaceSelectRangeSigned1) {
    testReplaceSelectRange<int16_t>(
        {{-4, 3}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 2u); });
}

TEST_F(P4CMidend, replaceSelectRangeSigned2) {
    testReplaceSelectRange<int16_t>(
        {{-256, 3}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 2u); });
}

TEST_F(P4CMidend, replaceSelectRangeSigned3) {
    testReplaceSelectRange<int16_t>(
        {{-256, 0}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 2u); });
}

TEST_F(P4CMidend, replaceSelectRangeSigned4) {
    testReplaceSelectRange<int16_t>(
        {{-256, -1}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 1u); });
}

TEST_F(P4CMidend, replaceSelectRangeSigned5) {
    testReplaceSelectRange<int16_t>(
        {{0, 15}}, [](CollectRangesAndMasks collect) { ASSERT_EQ(collect.masks.size(), 1u); });
}

}  // namespace Test
