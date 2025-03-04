/*
Copyright 2025-present Altera Corporation.

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

#include <gtest/gtest.h>

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "ir/splitter.h"

using namespace P4::literals;

namespace P4::Test {

struct SplitterTest : public ::testing::Test {
    SplitResult<IR::Statement> splitBefore(
        const IR::Statement *stat,
        std::function<bool(const IR::Statement *, const P4::Visitor_Context *)> predicate) {
        stat->apply(nameGen);
        return splitStatementBefore(stat, predicate, nameGen, &typeMap);
    }

    const IR::Statement *parse(std::string_view code, std::string_view decs = "") {
        const auto program = absl::StrCat(
            "extern void fn(); extern void f1(); extern void f2(); extern void f3(); ",
            "extern void f4(); extern void f5(); extern void f6(); extern void bar(); ",
            "control c() { bit<4> a; bit<4> b; bit<4> c; bit<4> d; bool bvar; ", decs, "apply {",
            code, "} }");
        const auto *prog = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
        CHECK_NULL(prog);
        P4::TypeInference ti(&typeMap, false, false, false);
        prog = prog->apply(ti);
        CHECK_NULL(prog);
        const IR::BlockStatement *bs = nullptr;
        P4::forAllMatching<IR::P4Control>(prog, [&](const auto *cntr) { bs = cntr->body; });
        CHECK_NULL(bs);
        return bs;
    }

    const IR::PathExpression *pe(std::string_view name) {
        return new IR::PathExpression(new IR::Path(P4::cstring(name)));
    }

    /// a = b
    const IR::AssignmentStatement *asgn(std::string_view lhs, std::string_view rhs) {
        return new IR::AssignmentStatement(pe(lhs), pe(rhs));
    }

    /// a = expr
    const IR::AssignmentStatement *asgn(std::string_view lhs, const IR::Expression *expr) {
        return new IR::AssignmentStatement(pe(lhs), expr);
    }

    /// a == b
    const IR::Equ *eq(std::string_view lhs, std::string_view rhs) {
        return new IR::Equ(pe(lhs), pe(rhs));
    }

    const IR::MethodCallStatement *call(std::string_view fn) {
        return new IR::MethodCallStatement(new IR::MethodCallExpression(pe(fn)));
    }

    const IR::BlockStatement *blk(IR::IndexedVector<IR::StatOrDecl> stmts) {
        return new IR::BlockStatement(std::move(stmts));
    }

    const IR::IfStatement *ifs(const IR::Expression *cond, const IR::Statement *tr,
                               const IR::Statement *fls = nullptr) {
        return new IR::IfStatement(cond, tr, fls);
    }

    MinimalNameGenerator nameGen;
    TypeMap typeMap;
};

#define EXPECT_EQUIV(a, b)                                                                        \
    EXPECT_TRUE(a->equiv(*b)) << "Actual:" << Log::indent << Log::endl                            \
                              << a << Log::unindent << "\nExpected: " << Log::indent << Log::endl \
                              << b << Log::unindent;

template <typename T>
bool predIs(const IR::Statement *stmt, const P4::Visitor::Context *) {
    return stmt->is<T>();
}

TEST_F(SplitterTest, SplitBsEmpty) {
    const auto *bs = blk({});
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);
    const auto *bbs = before->to<IR::BlockStatement>();
    ASSERT_TRUE(bbs) << before;
    EXPECT_EQ(bbs->components.size(), 0);
    EXPECT_FALSE(after);
    ASSERT_EQ(decls.size(), 0);
}

TEST_F(SplitterTest, SplitBsSimple1) {
    const auto *bs = blk({asgn("a", "b")});
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    const auto *bbs = before->to<IR::BlockStatement>();
    ASSERT_TRUE(bbs) << before;
    EXPECT_EQ(bbs->components.size(), 0) << bbs;

    const auto *abs = after->to<IR::BlockStatement>();
    ASSERT_TRUE(abs) << after;
    ASSERT_EQ(abs->components.size(), 1) << abs;
    EXPECT_TRUE(abs->components.front()->is<IR::AssignmentStatement>()) << abs;

    ASSERT_EQ(decls.size(), 0);
}

TEST_F(SplitterTest, SplitBsSimple2) {
    const auto *bs = blk({call("fn"), asgn("a", "b")});
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    const auto *bbs = before->to<IR::BlockStatement>();
    ASSERT_TRUE(bbs) << before;
    ASSERT_EQ(bbs->components.size(), 1) << before;
    EXPECT_TRUE(bbs->components.front()->is<IR::MethodCallStatement>()) << bbs;

    const auto *abs = after->to<IR::BlockStatement>();
    ASSERT_TRUE(abs) << after;
    ASSERT_EQ(abs->components.size(), 1) << abs;
    EXPECT_TRUE(abs->components.front()->is<IR::AssignmentStatement>()) << abs;

    ASSERT_EQ(decls.size(), 0);
}

TEST_F(SplitterTest, SplitBsIfSingleBranch) {
    const auto *bs = ifs(eq("a", "b"), blk({call("fn"), asgn("a", "b")}));
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    const auto *bbs = before->to<IR::BlockStatement>();
    ASSERT_TRUE(bbs) << before;
    ASSERT_EQ(bbs->components.size(), 2) << before;
    EXPECT_TRUE(bbs->components.at(0)->is<IR::AssignmentStatement>()) << bbs;
    const auto *bifs = bbs->components.at(1)->to<IR::IfStatement>();
    ASSERT_TRUE(bifs) << bbs;
    ASSERT_TRUE(bifs->ifTrue) << bifs;
    EXPECT_FALSE(bifs->ifFalse) << bifs;

    EXPECT_EQUIV(before, blk({asgn("cond", eq("a", "b")), ifs(pe("cond"), blk({call("fn")}))}));

    const auto *aifs = after->to<IR::IfStatement>();
    ASSERT_TRUE(aifs) << after;
    ASSERT_TRUE(aifs->ifTrue) << aifs;
    EXPECT_FALSE(aifs->ifFalse) << aifs;

    EXPECT_EQUIV(after, ifs(pe("cond"), blk({asgn("a", "b")})));

    ASSERT_EQ(decls.size(), 1);
    EXPECT_EQUIV(decls.front(), new IR::Declaration_Variable(IR::ID{"cond"_cs, nullptr},
                                                             IR::Type::Boolean::get()));
}

TEST_F(SplitterTest, SplitBsIfTwoBranches) {
    const auto *bs = ifs(eq("a", "b"), blk({call("fn"), asgn("a", "b")}), blk({asgn("c", "d")}));
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    EXPECT_EQUIV(before,
                 blk({asgn("cond", eq("a", "b")), ifs(pe("cond"), blk({call("fn")}), blk({}))}));

    EXPECT_EQUIV(after, ifs(pe("cond"), blk({asgn("a", "b")}), blk({asgn("c", "d")})));

    ASSERT_EQ(decls.size(), 1);
    EXPECT_EQUIV(decls.front(), new IR::Declaration_Variable(IR::ID{"cond"_cs, nullptr},
                                                             IR::Type::Boolean::get()));
}

TEST_F(SplitterTest, SplitBsIfNested) {
    const auto *bs = parse(R"(
if (a == b) {
    fn();
    if (bvar) {
        f2();
        a = b;
        c = b;
        f3();
    } else {
        f2();
        f4();
    }
    f5();
} else {
    c = d;
    bar();
})");
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    EXPECT_EQUIV(before, parse(R"(
{
    cond_0 = a == b;
    if (cond_0) {
        fn();
        {
            cond = bvar;
            if (cond) {
                f2();
                // cut
            } else {
                f2();
                f4();
            }
        }
    } else {
        // cut
    }
})",
                               "bool cond_0; bool cond;"));

    EXPECT_EQUIV(after, parse(R"(
if (cond_0) {
    if (cond) {
        // cut
        a = b;
        c = b;
        f3();
    }
    f5();
} else {
    // cut
    c = d;
    bar();
})",
                              "bool cond_0; bool cond;"));

    ASSERT_EQ(decls.size(), 2);
    EXPECT_EQUIV(decls.at(0), new IR::Declaration_Variable(IR::ID{"cond"_cs, nullptr},
                                                           IR::Type::Boolean::get()));
    EXPECT_EQUIV(decls.at(1), new IR::Declaration_Variable(IR::ID{"cond_0"_cs, nullptr},
                                                           IR::Type::Boolean::get()));
}

TEST_F(SplitterTest, IfSwitchNested) {
    const auto *bs = parse(R"(
if (a == b) {
    fn();
    switch (a) {
        0:
        1: { f2(); } // test we don't create fallthrough in "after"
        2: { f3(); a = b; f4(); }
        3: {}
        // test non-block in IF
        default: { if (a > 5) a = c; else f6(); }
    }
    f5();
} else {
    bar();
})");
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::AssignmentStatement>);
    ASSERT_TRUE(before);

    EXPECT_EQUIV(before, parse(R"(
{
    cond_0 = a == b;
    if (cond_0) {
        fn();
        {
            selector = a;
            switch (selector) {
                0:
                1: { f2(); }
                2: { f3(); }
                3: {}
                default: {
                    {
                        cond = a > 5;
                        if (cond) {}
                        else f6();
                    }
                }
            }
        }
    } else {
        bar();
    }
})",
                               "bool cond_0; bool cond; bit<4> selector; "));

    EXPECT_EQUIV(after, parse(R"(
if (cond_0) {
    switch (selector) {
        0:
        1: {}
        2: { a = b; f4(); }
        3: {}
        default: { if (cond) a = c; }
    }
    f5();
}
)",
                              "bool cond; bool cond_0; bit<4> selector; "));

    ASSERT_EQ(decls.size(), 3);
    EXPECT_EQUIV(decls.at(0), new IR::Declaration_Variable(IR::ID{"cond"_cs, nullptr},
                                                           IR::Type::Boolean::get()));
    EXPECT_EQUIV(decls.at(1), new IR::Declaration_Variable(IR::ID{"selector"_cs, nullptr},
                                                           IR::Type::Bits::get(4)));
    EXPECT_EQUIV(decls.at(2), new IR::Declaration_Variable(IR::ID{"cond_0"_cs, nullptr},
                                                           IR::Type::Boolean::get()));
}

TEST_F(SplitterTest, HoistVarIf) {
    const auto *bs = parse(R"(
if (a == b) {
    bit<4> x;
    bit<4> y;
    y = c + 4;
    x = y + 2;
    // split
    f1();
    x = c > a ? x + c : x + a;
})");
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::MethodCallStatement>);
    ASSERT_TRUE(before);

    EXPECT_EQUIV(before, parse(R"(
{
    cond = a == b;
    if (cond) {
        bit<4> y;
        y = c + 4;
        x = y + 2;
        // split
    }
})",
                               "bool cond; bit<4> x; "));

    EXPECT_EQUIV(after, parse(R"(
if (cond) {
    f1();
    x = c > a ? x + c : x + a;
})",
                              "bool cond; bit<4> x; "));

    ASSERT_EQ(decls.size(), 2);
    EXPECT_EQUIV(decls.at(0),
                 new IR::Declaration_Variable(IR::ID{"x"_cs, nullptr}, IR::Type::Bits::get(4)));
    EXPECT_EQUIV(decls.at(1), new IR::Declaration_Variable(IR::ID{"cond"_cs, nullptr},
                                                           IR::Type::Boolean::get()));
}

TEST_F(SplitterTest, HoistVarSwitch) {
    const auto *bs = parse(R"(
switch (a) {
    0: {
        bit<4> x;
        bit<4> y;
        y = c + 4;
        x = y + 2;
        // split
        f1();
        x = c > a ? x + c : x + a;
    }
    1: { a = b; }
})");
    auto [before, after, decls] = splitBefore(bs, &predIs<IR::MethodCallStatement>);
    ASSERT_TRUE(before);

    EXPECT_EQUIV(before, parse(R"(
{
    selector = a;
    switch (selector) {
        0: {
            bit<4> y;
            y = c + 4;
            x = y + 2;
            // split
        }
        1: { a = b; }
    }
})",
                               "bit<4> selector; bit<4> x; "));

    EXPECT_EQUIV(after, parse(R"(
switch (selector) {
    0: {
        f1();
        x = c > a ? x + c : x + a;
    }
    1: {}
})",
                              "bit<4> selector; bit<4> x; "));

    ASSERT_EQ(decls.size(), 2);
    EXPECT_EQUIV(decls.at(0),
                 new IR::Declaration_Variable(IR::ID{"x"_cs, nullptr}, IR::Type::Bits::get(4)));
    EXPECT_EQUIV(decls.at(1), new IR::Declaration_Variable(IR::ID{"selector"_cs, nullptr},
                                                           IR::Type::Bits::get(4)));
}

}  // namespace P4::Test
