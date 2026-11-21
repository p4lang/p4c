// SPDX-FileCopyrightText: 2024 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/toP4/toP4.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/log.h"
#include "midend/noMatch.h"
#include "midend/simplifySelectCases.h"

using namespace P4;

namespace P4::Test {

struct P4CFrontend : P4CTest {
    void addPasses(std::initializer_list<PassManager::VisitorRef> passes) { pm.addPasses(passes); }

    const IR::Node *parseAndProcess(std::string program) {
        const auto *pgm = P4::parseP4String(program);
        EXPECT_TRUE(pgm);
        EXPECT_EQ(::P4::errorCount(), 0);
        if (!pgm) {
            return nullptr;
        }
        return pgm->apply(pm);
    }

    PassManager pm;
};

struct P4CConstantFoldingValidation : P4CFrontend {
    void createPasses(P4::ConstantFoldingPolicy *policy) {
        addPasses({
            new P4::ConstantFolding(policy),
        });
    }
};

/**
 * Filter to prevent processing of fields in the "my_hdr_stack_1_t" struct
 */
class ConstantFoldFilter : public P4::ConstantFoldingPolicy {
 public:
    const IR::Node *hook(Visitor &v, IR::PathExpression *pe) override {
        auto ts = v.findContext<IR::Type_Struct>();
        if (ts && ts->name == "my_hdr_stack_1_t") {
            return pe;
        }
        return nullptr;
    }
};

const IR::StructField *getStructField(const IR::P4Program *program, cstring struct_name,
                                      cstring field_name) {
    for (const auto *d : program->getDeclarations()) {
        const auto *ts = d->to<IR::Type_Struct>();
        if (ts && ts->name == struct_name) {
            for (const auto *f : ts->fields) {
                const auto *sf = f->to<IR::StructField>();
                if (sf && sf->name == field_name) {
                    return sf;
                }
            }
        }
    }
    return nullptr;
}

const std::string constantFoldFilterCode() {
    static const std::string code = P4_SOURCE(R"(
        const bit<16> CONST_1 = 3;
        const bit<16> CONST_2 = 4;

        header my_hdr_t {
            bit<16>   field;
        }

        struct my_hdr_stack_1_t {
            my_hdr_t[CONST_1]    hdr;
        }

        struct my_hdr_stack_2_t {
            my_hdr_t[CONST_2]    hdr;
        }
    )");

    return code;
}

// Constant folding without any filter
TEST_F(P4CConstantFoldingValidation, no_filter) {
    // Create the passes without any ConstantFoldingPolicy
    createPasses(nullptr);

    auto *program = parseAndProcess(constantFoldFilterCode())->to<IR::P4Program>();
    ASSERT_TRUE(program);

    // my_hdr_stack_1_t.hdr should be a Constant
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Array>();
    ASSERT_TRUE(ts_1->size->is<IR::Constant>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Array>();
    ASSERT_TRUE(ts_2->size->is<IR::Constant>());
}

TEST_F(P4CConstantFoldingValidation, struct_cast_preserves_type) {
    createPasses(nullptr);

    std::string code = P4_SOURCE(R"(
        struct S {
            bit<8> x;
        }

        const S value = (S){x = 1};
    )");

    auto *program = parseAndProcess(code)->to<IR::P4Program>();
    ASSERT_TRUE(program);

    const IR::Declaration_Constant *constant = nullptr;
    for (const auto *declaration : program->getDeclarations()) {
        const auto *candidate = declaration->to<IR::Declaration_Constant>();
        if (candidate != nullptr && candidate->name == "value") {
            constant = candidate;
            break;
        }
    }
    ASSERT_TRUE(constant);

    const auto *initializer = constant->initializer->to<IR::StructExpression>();
    ASSERT_TRUE(initializer);
    ASSERT_TRUE(initializer->structType);
    EXPECT_TRUE(initializer->structType->is<IR::Type_Name>());
}

// Constant folding with filtering active
TEST_F(P4CConstantFoldingValidation, filter) {
    // Create the passes with a filter to skip processing of some structs
    createPasses(new ConstantFoldFilter());

    auto *program = parseAndProcess(constantFoldFilterCode())->to<IR::P4Program>();
    ASSERT_TRUE(program);

    // my_hdr_stack_1_t.hdr should be a Constant
    auto *sf_1 = getStructField(program, "my_hdr_stack_1_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_1);
    auto *ts_1 = sf_1->type->to<IR::Type_Array>();
    EXPECT_TRUE(ts_1->size->is<IR::PathExpression>());

    // my_hdr_stack_2_t.hdr should be a Constant
    auto *sf_2 = getStructField(program, "my_hdr_stack_2_t"_cs, "hdr"_cs);
    ASSERT_TRUE(sf_2);
    auto *ts_2 = sf_2->type->to<IR::Type_Array>();
    EXPECT_TRUE(ts_2->size->is<IR::Constant>());
}

// Run only type checking and folding so these tests observe specialization before later
// parser simplification can merge states or remove transitions.
struct P4CPartialSelect : P4CFrontend {
    TypeMap typeMap;
    const IR::P4Program *lastProgram = nullptr;

    P4CPartialSelect() {
        addPasses({new CreateBuiltins(), new TypeInference(&typeMap, false),
                   new TypeChecking(nullptr, &typeMap, true),
                   new DoConstantFolding(&typeMap, false)});
    }

    const IR::Expression *transition(const std::string &source) {
        const auto *program = parseAndProcess(source);
        if (!program) return nullptr;
        lastProgram = program->checkedTo<IR::P4Program>();
        const auto *parser = lastProgram->getDeclsByName("P"_cs)->single();
        if (!parser) return nullptr;
        const auto *start =
            parser->checkedTo<IR::P4Parser>()->states.getDeclaration<IR::ParserState>(
                IR::ParserState::start);
        if (!start || !start->selectExpression) return nullptr;
        return start->selectExpression;
    }

    const IR::SelectExpression *fold(const std::string &source) {
        const auto *result = transition(source);
        return result ? result->to<IR::SelectExpression>() : nullptr;
    }
};

TEST_F(P4CPartialSelect, MasksRangesAndPriority) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x) {
            state start {
                transition select(8w0x12, x, -8s3) {
                    (0x20 &&& 0xf0, _, _): wrong;
                    (0x10 &&& 0xf0, 0xa0 &&& 0xf0, -4 .. -1): first;
                    (_, 0xab, -3): second;
                    (0x12, 0x20 .. 0x2f, -3): third;
                    default: fallback;
                }
            }
            state wrong { transition reject; }
            state first { transition accept; }
            state second { transition accept; }
            state third { transition accept; }
            state fallback { transition accept; }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    EXPECT_EQ("x", result->select->components.at(0)->checkedTo<IR::PathExpression>()->path->name);
    ASSERT_EQ(4U, result->selectCases.size());
    EXPECT_EQ("first", result->selectCases.at(0)->state->path->name);
    EXPECT_EQ("second", result->selectCases.at(1)->state->path->name);
    EXPECT_EQ("third", result->selectCases.at(2)->state->path->name);

    // Independently evaluate the projected labels for every packet byte. This checks inclusive
    // range bounds, mask membership, defaults, and first-match precedence over the exact 0xab.
    for (unsigned x = 0; x < 256; ++x) {
        cstring actual;
        for (auto c : result->selectCases) {
            bool matches = c->keyset->is<IR::DefaultExpression>();
            if (auto k = c->keyset->to<IR::Constant>()) matches = x == k->asUnsigned();
            if (auto k = c->keyset->to<IR::Mask>()) {
                auto value = k->left->checkedTo<IR::Constant>()->asUnsigned();
                auto mask = k->right->checkedTo<IR::Constant>()->asUnsigned();
                matches = (x & mask) == (value & mask);
            }
            if (auto k = c->keyset->to<IR::Range>()) {
                matches = x >= k->left->checkedTo<IR::Constant>()->asUnsigned() &&
                          x <= k->right->checkedTo<IR::Constant>()->asUnsigned();
            }
            if (matches) {
                actual = c->state->path->name;
                break;
            }
        }
        cstring expected = (x & 0xf0) == 0xa0       ? "first"_cs
                           : x >= 0x20 && x <= 0x2f ? "third"_cs
                                                    : "fallback"_cs;
        EXPECT_EQ(expected, actual) << "packet byte " << x;
    }
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, BooleanEnumAndNestedConstant) {
    const auto *result = fold(P4_SOURCE(R"(
        enum Mode { A, B }
        parser P(in bit<8> x) {
            state start {
                transition select(true, x, Mode.A, {8w1, 8w2}) {
                    (false, _, _, _): reject;
                    (true, _, Mode.B, _): reject;
                    (_, 7, Mode.A, {1, 2}): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    EXPECT_EQ(7, result->selectCases.at(0)->keyset->checkedTo<IR::Constant>()->asInt());
    EXPECT_TRUE(result->selectCases.at(1)->keyset->is<IR::DefaultExpression>());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, SerializableEnumRepresentation) {
    const auto *result = fold(P4_SOURCE(R"(
        enum bit<8> Protocol { TCP = 6, UDP = 17 }
        parser P(in bit<8> x) {
            state start {
                transition select(Protocol.TCP, x) {
                    (Protocol.UDP, 1): reject;
                    (Protocol.TCP, 2): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    EXPECT_EQ(2, result->selectCases.at(0)->keyset->checkedTo<IR::Constant>()->asInt());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, EnumTypedefIdentityAndPriority) {
    const auto *result = fold(P4_SOURCE(R"(
        enum Mode { A, B }
        typedef Mode Alias;
        typedef Alias OtherAlias;
        parser P(in bit<8> x) {
            state start {
                transition select(Mode.A, x, Alias.A) {
                    (Alias.B, _, _): reject;
                    (Alias.A, 7, Mode.A): first;
                    (OtherAlias.A, 7, Alias.A): second;
                    (Mode.A, 9, OtherAlias.A): third;
                    default: reject;
                }
            }
            state first { transition accept; }
            state second { transition reject; }
            state third { transition accept; }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(4U, result->selectCases.size());
    EXPECT_EQ("first", result->selectCases.at(0)->state->path->name);
    EXPECT_EQ("second", result->selectCases.at(1)->state->path->name);
    EXPECT_EQ("third", result->selectCases.at(2)->state->path->name);
    EXPECT_TRUE(result->selectCases.at(3)->keyset->is<IR::DefaultExpression>());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, SerializableEnumRangesAndMasks) {
    const auto *result = fold(P4_SOURCE(R"(
        enum bit<8> E { A = 1, B = 2, C = 3, SameAsB = 2 }
        typedef E Alias;
        parser P(in bit<8> x) {
            state start {
                transition select(Alias.B, x) {
                    (E.A .. Alias.C, 7): accept;
                    (Alias.B &&& E.C, 8): accept;
                    (0 .. 3, 9): accept;
                    (2 &&& 3, 10): accept;
                    (E.SameAsB, 11): accept;
                    (E.C .. E.C, _): reject;
                    (E.C &&& E.C, _): reject;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(6U, result->selectCases.size());
    for (unsigned i = 0; i < 5; ++i)
        EXPECT_EQ(7 + i,
                  result->selectCases.at(i)->keyset->checkedTo<IR::Constant>()->asUnsigned());
    EXPECT_TRUE(result->selectCases.at(5)->keyset->is<IR::DefaultExpression>());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, OpaqueTupleConstructorParameter) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x)(tuple<bit<8>, bit<8>> key) {
            state start {
                transition select({8w1, 8w2}, x, 8w9) {
                    (key, 7, 9): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    // Defer the opaque tuple comparison, while still projecting the independent last key.
    ASSERT_EQ(2U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    EXPECT_TRUE(result->select->components.at(0)->is<IR::ListExpression>());
    const auto *key = result->selectCases.at(0)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(2U, key->size());
    EXPECT_EQ("key", key->components.at(0)->checkedTo<IR::PathExpression>()->path->name);
    lastProgram = lastProgram->apply(TypeChecking(nullptr, &typeMap, true));
    ASSERT_EQ(0U, ::P4::errorCount());
    const auto *again = lastProgram->apply(ConstantFolding(&typeMap, false));
    EXPECT_TRUE(lastProgram->equiv(*again));
    Util::SourceCodeBuilder builder;
    lastProgram->apply(ToP4(builder, false));
    const auto *reparsed = parseP4String(builder.toString());
    ASSERT_TRUE(reparsed);
    TypeMap reparsedTypes;
    PassManager check({new CreateBuiltins(), new TypeChecking(nullptr, &reparsedTypes, true)});
    reparsed->apply(check);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, FullyConstantSerializableEnumSets) {
    const auto *result = transition(P4_SOURCE(R"(
        enum bit<8> E { A = 1, B = 2, C = 3 }
        typedef E Alias;
        parser P() {
            state start {
                transition select(Alias.B) {
                    E.C .. Alias.C: reject;
                    E.A &&& E.C: reject;
                    E.A .. Alias.C: match;
                    default: reject;
                }
            }
            state match { transition accept; }
        }
    )"));
    ASSERT_TRUE(result);
    EXPECT_EQ("match", result->checkedTo<IR::PathExpression>()->path->name);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, IntegerBoundariesAndWideMasks) {
    const auto *result = fold(P4_SOURCE(R"(
        enum int<8> Signed { Low = -128, High = 127, Negative = -1 }
        parser P(in bit<8> x) {
            state start {
                transition select(Signed.Low, (int<8>)127, Signed.Negative, 8w255,
                                  128w0xffffffffffffffffffffffffffffffff, x) {
                    (Signed.Low .. Signed.Low, 127 .. 127, -1 .. 1, 255 .. 255,
                     128w0 &&& 128w0, 7): first;
                    (-128 &&& -1, 0 .. 127, -128 .. 0, 255 &&& 255,
                     128w0xffffffffffffffffffffffffffffffff &&&
                         128w0xffffffffffffffffffffffffffffffff, 7): second;
                    (-127 .. 127, _, _, _, _, _): reject;
                    (_, -128 .. 126, _, _, _, _): reject;
                    (_, _, 0 .. 127, _, _, _): reject;
                    (_, _, _, 0 .. 254, _, _): reject;
                    (_, _, _, _, 128w0 &&& 128w0xffffffffffffffffffffffffffffffff, _): reject;
                    default: reject;
                }
            }
            state first { transition accept; }
            state second { transition reject; }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(3U, result->selectCases.size());
    EXPECT_EQ("first", result->selectCases.at(0)->state->path->name);
    EXPECT_EQ("second", result->selectCases.at(1)->state->path->name);
    EXPECT_TRUE(result->selectCases.at(2)->keyset->is<IR::DefaultExpression>());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, RetainedNestedTupleKeepsItsType) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x, in bit<8> y, in tuple<bit<8>, bit<8>> key) {
            state start {
                transition select(8w1, {x, y}) {
                    (1, {7, 9}): accept;
                    (1, key): reject;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(3U, result->selectCases.size());
    const auto *key = result->selectCases.at(0)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(1U, key->size());
    EXPECT_TRUE(key->components.at(0)->is<IR::ListExpression>());
    const auto *runtimeKey = result->selectCases.at(1)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(1U, runtimeKey->size());
    EXPECT_TRUE(runtimeKey->components.at(0)->is<IR::PathExpression>());
    lastProgram = lastProgram->apply(TypeChecking(nullptr, &typeMap, true));
    ASSERT_EQ(0U, ::P4::errorCount());

    // Source export is a consumer too: preserve the singleton outer tuple when reparsing.
    Util::SourceCodeBuilder builder;
    lastProgram->apply(ToP4(builder, false));
    const auto *reparsed = parseP4String(builder.toString());
    ASSERT_TRUE(reparsed);
    TypeMap reparsedTypes;
    PassManager check({new CreateBuiltins(), new TypeInference(&reparsedTypes, false)});
    reparsed->apply(check);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, InteriorDimensionAndRepeatedFolding) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x, in bit<8> y) {
            state start {
                transition select(x, 8w1, y) {
                    (7, 1, 9): accept;
                    (7, 2, 10): reject;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(2U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    const auto *key = result->selectCases.at(0)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(2U, key->size());
    EXPECT_EQ(7, key->components.at(0)->checkedTo<IR::Constant>()->asInt());
    EXPECT_EQ(9, key->components.at(1)->checkedTo<IR::Constant>()->asInt());
    const auto *again = lastProgram->apply(ConstantFolding(&typeMap, false));
    EXPECT_TRUE(lastProgram->equiv(*again));
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, ProjectedWildcardPreservesFirstMatch) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x) {
            state start {
                transition select(8w1, x) {
                    (1, _): first;
                    (1, 7): reject;
                    default: reject;
                }
            }
            state first { transition accept; }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->selectCases.size());
    EXPECT_TRUE(result->selectCases.at(0)->keyset->is<IR::DefaultExpression>());
    EXPECT_EQ("first", result->selectCases.at(0)->state->path->name);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, RuntimeValueSetKeepsSchema) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x) {
            value_set<tuple<bit<8>, bit<8>>>(4) runtime_keys;
            state start {
                transition select(8w1, x) {
                    runtime_keys: accept;
                    (2, 7): reject;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    EXPECT_EQ(2U, result->select->size());
    ASSERT_EQ(3U, result->selectCases.size());
    EXPECT_EQ("runtime_keys",
              result->selectCases.at(0)->keyset->checkedTo<IR::PathExpression>()->path->name);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, RuntimeKeysetKeepsDependentDimension) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x, in bit<8> y) {
            state start {
                transition select(8w1, x, 8w2) {
                    (y, 7, 2): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    // The last dimension can be removed, but the first still depends on a runtime keyset.
    EXPECT_EQ(2U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    const auto *key = result->selectCases.at(0)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(2U, key->size());
    EXPECT_EQ("y", key->components.at(0)->checkedTo<IR::PathExpression>()->path->name);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, EmptySelectPreservesNoMatch) {
    const auto *result = fold(P4_SOURCE(P4Headers::CORE, R"(
        parser P(in bit<8> x) {
            state start {
                transition select(8w1, x) {
                    (2, 7): accept;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    EXPECT_TRUE(result->select->components.at(0)->is<IR::PathExpression>());
    EXPECT_TRUE(result->selectCases.empty());
    EXPECT_EQ(0U, ::P4::errorCount());

    // Exercise the consumer of an empty select, including the exact parser error.
    PassManager lower({new TypeChecking(nullptr, &typeMap),
                       new SimplifySelectCases(&typeMap, false), new HandleNoMatch(),
                       new TypeChecking(nullptr, &typeMap, true)});
    const auto *lowered = lastProgram->apply(lower);
    ASSERT_TRUE(lowered);
    class FindNoMatch : public Inspector {
     public:
        unsigned count = 0;
        void postorder(const IR::MethodCallExpression *call) override {
            const auto *path = call->method->to<IR::PathExpression>();
            if (!path || path->path->name != "verify") return;
            ASSERT_EQ(2U, call->arguments->size());
            EXPECT_FALSE(call->arguments->at(0)->expression->checkedTo<IR::BoolLiteral>()->value);
            EXPECT_EQ("NoMatch",
                      call->arguments->at(1)->expression->checkedTo<IR::Member>()->member.name);
            ++count;
        }
    } findNoMatch;
    lowered->apply(findNoMatch);
    EXPECT_EQ(1U, findNoMatch.count);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, DeepSingletonTupleRoundTrip) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x, in bit<8> y) {
            state start {
                transition select({}, {{8w1, 8w2}}, {{x}, y}) {
                    ({}, {{1, 2}}, {{7}, 9}): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    ASSERT_EQ(1U, result->select->size());
    ASSERT_EQ(2U, result->selectCases.size());
    const auto *key = result->selectCases.at(0)->keyset->checkedTo<IR::ListExpression>();
    ASSERT_EQ(1U, key->size());
    const auto *tuple = key->components.at(0)->checkedTo<IR::ListExpression>();
    ASSERT_EQ(2U, tuple->size());
    EXPECT_TRUE(tuple->components.at(0)->is<IR::ListExpression>());
    lastProgram = lastProgram->apply(TypeChecking(nullptr, &typeMap, true));
    const auto *again = lastProgram->apply(ConstantFolding(&typeMap, false));
    EXPECT_TRUE(lastProgram->equiv(*again));
    Util::SourceCodeBuilder builder;
    lastProgram->apply(ToP4(builder, false));
    const auto *reparsed = parseP4String(builder.toString());
    ASSERT_TRUE(reparsed);
    TypeMap reparsedTypes;
    PassManager check({new CreateBuiltins(), new TypeChecking(nullptr, &reparsedTypes, true)});
    reparsed->apply(check);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, EffectfulSelectWaitsForSideEffectOrdering) {
    const auto *result = fold(P4_SOURCE(R"(
        extern bit<8> read();
        parser P() {
            state start {
                transition select(8w1, read()) {
                    (2, 7): accept;
                    default: next;
                }
            }
            state next { transition accept; }
        }
    )"));
    ASSERT_TRUE(result);
    EXPECT_EQ(2U, result->select->size());
    EXPECT_EQ(2U, result->selectCases.size());
    EXPECT_TRUE(result->select->components.at(1)->is<IR::MethodCallExpression>());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CFrontend, PartialSelectWithTypeMapOnly) {
    TypeMap typeMap;
    // ConstantFolding's API requires a TypeMap, not types embedded in expressions.
    addPasses({new CreateBuiltins(), new TypeInference(&typeMap, false),
               new ConstantFolding(&typeMap, false)});
    const auto *program = parseAndProcess(P4_SOURCE(R"(
        extern bit<8> read();
        parser P() {
            state start {
                transition select(8w1, read()) {
                    (2, 7): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(program);
    const auto *parser = program->checkedTo<IR::P4Program>()
                             ->getDeclsByName("P"_cs)
                             ->single()
                             ->checkedTo<IR::P4Parser>();
    const auto *start = parser->states.getDeclaration<IR::ParserState>(IR::ParserState::start);
    ASSERT_TRUE(start);
    const auto *select = start->selectExpression->checkedTo<IR::SelectExpression>();
    EXPECT_EQ(2U, select->select->size());
    EXPECT_TRUE(select->select->components.at(1)->is<IR::MethodCallExpression>());
    EXPECT_EQ(2U, select->selectCases.size());
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, FrontendPreservesEffectfulEvaluation) {
    auto test = FrontendTestCase::create(P4_SOURCE(P4Headers::CORE, R"(
        extern bit<8> first(inout bit<8> x);
        extern bit<8> second(inout bit<8> x);
        parser P() {
            bit<8> x = 0;
            state start {
                transition select(8w1, first(x), second(x)) {
                    (2, 7, 8): accept;
                    default: next;
                }
            }
            state next { transition accept; }
        }
        parser PT();
        package Top(PT p);
        Top(P()) main;
    )"));
    ASSERT_TRUE(test);
    class FindCalls : public Inspector {
     public:
        std::vector<cstring> calls;
        void postorder(const IR::MethodCallExpression *call) override {
            auto path = call->method->to<IR::PathExpression>();
            if (path && (path->path->name == "first" || path->path->name == "second")) {
                calls.push_back(path->path->name);
                EXPECT_FALSE(isInContext<IR::SelectExpression>());
            }
        }
    } findCalls;
    test->program->apply(findCalls);
    EXPECT_EQ((std::vector<cstring>{"first"_cs, "second"_cs}), findCalls.calls);
    EXPECT_EQ(0U, ::P4::errorCount());
}

TEST_F(P4CPartialSelect, DynamicSelectIsUnchanged) {
    const auto *result = fold(P4_SOURCE(R"(
        parser P(in bit<8> x, in bit<8> y) {
            state start {
                transition select(x, y) {
                    (1, 7): accept;
                    default: reject;
                }
            }
        }
    )"));
    ASSERT_TRUE(result);
    EXPECT_EQ(2U, result->select->size());
    EXPECT_EQ(2U, result->selectCases.size());
    EXPECT_EQ(0U, ::P4::errorCount());
}

}  // namespace P4::Test
