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

#include <map>

#include "gtest/gtest.h"
#include "backends/bmv2/helpers.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"
#include "ir/ir.h"
#include "midend/simplifyKey.h"
#include "midend/synthesizeValidField.h"

namespace BMV2 {

class BMV2_SynthesizeValidField : public ::Test::P4CTest { };

// Verify that SynthesizeValidField adds a header validity bit field only to
// header types (and not header unions or structs).
TEST_F(BMV2_SynthesizeValidField, ValidField) {
    auto source = P4_SOURCE(P4Headers::CORE, R"(
        header H1 { bit<8> field; }
        header_union U1 { H1 h1; }
        struct S1 { H1 h1; }
    )");

    auto program =
      P4::parseP4String(source, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    PassManager passes = {
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SynthesizeValidField(&refMap, &typeMap),
    };
    program = program->apply(passes);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    forAllMatching<IR::Type_Header>(program, [](const IR::Type_Header* h) {
        EXPECT_TRUE(h->getField(V1ModelProperties::validField) != nullptr);
    });
    forAllMatching<IR::Type_HeaderUnion>(program, [](const IR::Type_HeaderUnion* h) {
        EXPECT_TRUE(h->getField(V1ModelProperties::validField) == nullptr);
    });
    forAllMatching<IR::Type_Struct>(program, [](const IR::Type_Struct* h) {
        EXPECT_TRUE(h->getField(V1ModelProperties::validField) == nullptr);
    });
}

// Verify that SynthesizeValidField converts 'isValid()' calls on headers to
// references to the valid bit, but leaves 'isValid()' calls on header unions
// untouched.
TEST_F(BMV2_SynthesizeValidField, Expressions) {
    auto source = P4_SOURCE(P4Headers::CORE, R"(
        header H1 { bit<8> field; }
        header H2 { bit<3> field; }
        header_union U1 { H1 h1; H2 h2; }

        control c(inout U1 u1) {
            action _header() { if (u1.h1.isValid()) u1.h1.field = 8w0; }
            action _header_union() { if (u1.isValid()) u1.h1.field = 8w0; }
            action _both() { bool b = !u1.isValid() || !u1.h1.isValid(); }
            table t {
                key = { u1.h1.field : exact; }
                actions = { _header; _header_union; _both; }
                default_action = _both;
            }
            apply { t.apply(); }
        }
    )");

    auto program =
      P4::parseP4String(source, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    PassManager passes = {
        new P4::CreateBuiltins(),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SynthesizeValidField(&refMap, &typeMap),
    };
    program = program->apply(passes);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    auto countValidFieldRefs = [&](const IR::Node* node) {
        unsigned count = 0;
        forAllMatching<IR::Member>(node, [&](const IR::Member* member) {
            if (member->member.name != V1ModelProperties::validField) return;
            ++count;
        });
        return count;
    };

    auto countIsValidCalls = [&](const IR::Node* node) {
        unsigned count = 0;
        forAllMatching<IR::MethodCallExpression>(node,
                [&](const IR::MethodCallExpression* call) {
            auto instance = P4::MethodInstance::resolve(call, &refMap, &typeMap);
            if (!instance->is<P4::BuiltInMethod>()) return;
            auto builtin = instance->to<P4::BuiltInMethod>();
            if (IR::Type_Header::isValid != builtin->name.name) return;
            ++count;
        });
        return count;
    };

    forAllMatching<IR::P4Action>(program, [&](const IR::P4Action* action) {
        if (action->name.name == "_header") {
            EXPECT_EQ(1u, countValidFieldRefs(action));
            EXPECT_EQ(0u, countIsValidCalls(action));
        } else if (action->name.name == "_header_union") {
            EXPECT_EQ(0u, countValidFieldRefs(action));
            EXPECT_EQ(1u, countIsValidCalls(action));
        } else if (action->name.name == "_both") {
            EXPECT_EQ(1u, countValidFieldRefs(action));
            EXPECT_EQ(1u, countIsValidCalls(action));
        } else if (action->name.name != "NoAction") {
            ADD_FAILURE() << "Unexpected action: " << action->name.name;
        }
    });

    program = program->apply(P4::TypeChecking(&refMap, &typeMap));
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());
}

// Verify that SynthesizeValidField converts matches against 'isValid()' on
// headers (but not header unions) to matches against their valid bit.
TEST_F(BMV2_SynthesizeValidField, MatchKeys) {
    auto source = P4_SOURCE(P4Headers::CORE, R"(
        header H1 { bit<8> field; }
        header H2 { bit<3> field; }
        header_union U1 { H1 h1; H2 h2; }

        control c(inout U1 u1) {
            action noop() { }
            table t {
                key = {
                    u1.h1.isValid() : exact;
                    u1.h1.isValid() : ternary;
                    u1.h2.isValid() : exact;
                    u1.h2.isValid() : ternary;
                    u1.isValid() : exact;
                    u1.isValid() : ternary;
                    u1.h1.field : exact;
                    u1.h2.field : exact;
                }
                actions = { noop; }
                default_action = noop;
            }
            apply { t.apply(); }
        }
    )");

    auto program =
      P4::parseP4String(source, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    PassManager passes = {
        new P4::CreateBuiltins(),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SynthesizeValidField(&refMap, &typeMap),
    };
    program = program->apply(passes);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    auto expectValidField = [&](const IR::Node* node) {
        ASSERT_TRUE(node->is<IR::Member>());
        EXPECT_STREQ(V1ModelProperties::validField.c_str(),
                     node->to<IR::Member>()->member.name.c_str());
    };

    auto expectIsValidBuiltin = [&](const IR::Node* node) {
        ASSERT_TRUE(node->is<IR::MethodCallExpression>());
        auto call = node->to<IR::MethodCallExpression>();
        auto instance = P4::MethodInstance::resolve(call, &refMap, &typeMap);
        ASSERT_TRUE(instance->is<P4::BuiltInMethod>());
        auto builtin = instance->to<P4::BuiltInMethod>();
        EXPECT_STREQ(IR::Type_Header::isValid.c_str(), builtin->name.name.c_str());
        auto type = typeMap.getType(builtin->appliedTo, true);
        EXPECT_TRUE(type->is<IR::Type_HeaderUnion>());
    };

    auto expectNormalField = [&](const IR::Node* node) {
        ASSERT_TRUE(node->is<IR::Member>());
        EXPECT_STREQ("field", node->to<IR::Member>()->member.name.c_str());
    };

    forAllMatching<IR::Key>(program, [&](const IR::Key* key) {
        ASSERT_EQ(8u, key->keyElements.size());
        expectValidField(key->keyElements[0]->expression);
        expectValidField(key->keyElements[1]->expression);
        expectValidField(key->keyElements[2]->expression);
        expectValidField(key->keyElements[3]->expression);
        expectIsValidBuiltin(key->keyElements[4]->expression);
        expectIsValidBuiltin(key->keyElements[5]->expression);
        expectNormalField(key->keyElements[6]->expression);
        expectNormalField(key->keyElements[7]->expression);
    });

    program = program->apply(P4::TypeChecking(&refMap, &typeMap));
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    auto expectType = [&](const IR::Node* node, const IR::Type* type) {
        auto nodeType = typeMap.getType(node, true);
        EXPECT_EQ(type, nodeType);
    };

    forAllMatching<IR::Key>(program, [&](const IR::Key* key) {
        ASSERT_EQ(8u, key->keyElements.size());
        expectType(key->keyElements[0]->expression, IR::Type::Bits::get(1));
        expectType(key->keyElements[1]->expression, IR::Type::Bits::get(1));
        expectType(key->keyElements[2]->expression, IR::Type::Bits::get(1));
        expectType(key->keyElements[3]->expression, IR::Type::Bits::get(1));
        expectType(key->keyElements[4]->expression, IR::Type::Boolean::get());
        expectType(key->keyElements[5]->expression, IR::Type::Boolean::get());
        expectType(key->keyElements[6]->expression, IR::Type::Bits::get(8));
        expectType(key->keyElements[7]->expression, IR::Type::Bits::get(3));
    });
}

// Verify that when SynthesizeValidField rewrites 'isValid()' calls in table
// match keys, which changes their type from bool to bit<1>, that it also
// rewrites the keys of const table entries so that the program is still
// correctly typed.
TEST_F(BMV2_SynthesizeValidField, ConstTableEntries) {
    auto source = P4_SOURCE(P4Headers::CORE, R"(
        header H1 { bit<8> field; }
        header H2 { bit<3> field; }
        header_union U1 { H1 h1; H2 h2; }

        control c(inout U1 u1) {
            action noop() { }
            table t {
                key = {
                    u1.h1.isValid() : exact;
                    u1.h2.isValid() : ternary;
                    u1.isValid() : exact;
                    u1.isValid() : ternary;
                }
                actions = { noop; }
                default_action = noop;
                const entries = {
                    (true, true, true, true) : noop();
                    (false, false, false, false) : noop();
                }
            }
            apply { t.apply(); }
        }
    )");

    auto program =
      P4::parseP4String(source, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    PassManager passes = {
        new P4::CreateBuiltins(),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SynthesizeValidField(&refMap, &typeMap),
    };
    program = program->apply(passes);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    forAllMatching<IR::EntriesList>(program, [&](const IR::EntriesList* el) {
        ASSERT_EQ(2u, el->size());

        auto trues = el->entries[0]->keys->components;
        ASSERT_EQ(4u, trues.size());
        ASSERT_TRUE(trues[0]->is<IR::Constant>());
        EXPECT_EQ(1, trues[0]->to<IR::Constant>()->asInt());
        ASSERT_TRUE(trues[1]->is<IR::Constant>());
        EXPECT_EQ(1, trues[1]->to<IR::Constant>()->asInt());
        ASSERT_TRUE(trues[2]->is<IR::BoolLiteral>());
        EXPECT_EQ(true, trues[2]->to<IR::BoolLiteral>()->value);
        ASSERT_TRUE(trues[3]->is<IR::BoolLiteral>());
        EXPECT_EQ(true, trues[3]->to<IR::BoolLiteral>()->value);

        auto falses = el->entries[1]->keys->components;
        ASSERT_EQ(4u, falses.size());
        ASSERT_TRUE(falses[0]->is<IR::Constant>());
        EXPECT_EQ(0, falses[0]->to<IR::Constant>()->asInt());
        ASSERT_TRUE(falses[1]->is<IR::Constant>());
        EXPECT_EQ(0, falses[1]->to<IR::Constant>()->asInt());
        ASSERT_TRUE(falses[2]->is<IR::BoolLiteral>());
        ASSERT_FALSE(falses[2]->to<IR::BoolLiteral>()->value);
        ASSERT_TRUE(falses[3]->is<IR::BoolLiteral>());
        ASSERT_FALSE(falses[3]->to<IR::BoolLiteral>()->value);
    });

    program = program->apply(P4::TypeChecking(&refMap, &typeMap));
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());
}

// Verify that the combination of SynthesizeValidField (which should remove the
// header form of 'isValid()' from match keys) and SimplifyKey (which should
// remove the header union from of 'isValid()' from match keys, given the
// appropriate policy) results in a program with no 'isValid()' calls in match
// keys.
TEST_F(BMV2_SynthesizeValidField, SimplifiedKeysHaveNoIsValid) {
    auto source = P4_SOURCE(P4Headers::CORE, R"(
        header H1 { bit<8> field; }
        header H2 { bit<3> field; }
        header_union U1 { H1 h1; H2 h2; }

        control c(inout U1 u1) {
            action noop() { }
            table t {
                key = {
                    u1.h1.isValid() : exact;
                    u1.h1.isValid() : ternary;
                    u1.h2.isValid() : exact;
                    u1.h2.isValid() : ternary;
                    u1.isValid() : exact;
                    u1.isValid() : ternary;
                    u1.h1.field : exact;
                    u1.h2.field : exact;
                }
                actions = { noop; }
                default_action = noop;
            }
            apply { t.apply(); }
        }
    )");

    auto program =
      P4::parseP4String(source, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    PassManager passes = {
        new P4::CreateBuiltins(),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SynthesizeValidField(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::OrPolicy(
                                new P4::IsValid(&refMap, &typeMap),
                                new P4::IsMask()))
    };
    program = program->apply(passes);
    ASSERT_TRUE(program != nullptr);
    ASSERT_EQ(0u, ::diagnosticCount());

    forAllMatching<IR::KeyElement>(program, [&](const IR::KeyElement* elem) {
        EXPECT_TRUE(!elem->expression->is<IR::MethodCallExpression>());
    });
}

}  // namespace BMV2
