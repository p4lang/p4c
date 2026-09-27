// Copyright 2024 Intel Corp.
// SPDX-FileCopyrightText: 2024 Intel Corp.
//
// SPDX-License-Identifier: Apache-2.0

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "midend_pass.h"

namespace P4::Test {

using P4TestContext = P4CContextWithOptions<CompilerOptions>;

class P4CVisitor : public P4CTest {};

struct MultiVisitInspector : public Inspector, virtual public P4::ResolutionContext {
    MultiVisitInspector() { visitDagOnce = false; }

 private:
    // ignore parser and declaration loops for now
    void loop_revisit(const IR::ParserState *) override {}

    void visit_def(const IR::PathExpression *pe) {
        auto d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
        BUG_CHECK(d, "failed to resolve %s", pe);
        if (auto *ps = d->to<IR::ParserState>()) {
            visit(ps, "transition");
        } else if (auto *act = d->to<IR::P4Action>()) {
            visit(act, "actions");
        } else {
            auto *obj = d->getNode();  // FIXME -- should be able to visit an INode
            visit(obj);
        }
    }

    bool preorder(const IR::PathExpression *path) override {
        visit_def(path);
        return true;
    }

    MultiVisitInspector(const MultiVisitInspector &) = default;
};

struct MultiVisitModifier : public Modifier,
                            // protected ControlFlowVisitor,
                            virtual public P4::ResolutionContext {
    MultiVisitModifier() { visitDagOnce = false; }

 private:
    // ignore parser and declaration loops for now
    void loop_revisit(const IR::ParserState *) override {}

    void visit_def(const IR::PathExpression *pe) {
        auto d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
        BUG_CHECK(d, "failed to resolve %s", pe);
        if (auto *ps = d->to<IR::ParserState>()) {
            visit(ps, "transition");
        } else if (auto *act = d->to<IR::P4Action>()) {
            visit(act, "actions");
        } else {
            auto *obj = d->getNode();  // FIXME -- should be able to visit an INode
            visit(obj);
        }
    }

    bool preorder(IR::PathExpression *path) override {
        visit_def(path);
        return true;
    }

    MultiVisitModifier(const MultiVisitModifier &) = default;
};

std::string getMultiVisitLoopSource() {
    // Non-sensical parser with loops
    return R"(
        extern packet_in {
            void extract<T> (out T hdr);
        }

        header Header {
            bit<32> data;
        }

        struct H {
            Header h1;
            Header h2;
        }

        struct M { }

        parser MyParser(packet_in pkt, out H hdr, out M meta) {
            state start {
                transition next;
            }

            state next {
                transition select (hdr.h1.data) {
                    0: state0;
                    1: state1;
                    default: accept;
                }
            }

            state state0 {
                hdr.h1.setInvalid();
                transition select (hdr.h2.data) {
                    2: state1;
                    default: next;
                }
            }

            state state1 {
                hdr.h2.setValid();
                transition select (hdr.h2.data) {
                    1: state1;
                    2: state0;
                    default: accept;
                }
            }

            state accept { }
        }
    )";
}

// This test fails when Visitor::Tracker::try_start does _not_ reset done on a previously-visited
// node
TEST_F(P4CVisitor, MultiVisitInspectorLoop) {
    auto program = P4::parseP4String(getMultiVisitLoopSource());
    ASSERT_TRUE(program != nullptr);

    program = program->apply(MultiVisitInspector());
    ASSERT_TRUE(program != nullptr);
}

// This test fails when Visitor::ChangeTracker::try_start does _not_ reset visit_in_progress on a
// previously-visited node
TEST_F(P4CVisitor, MultiVisitModifierLoop) {
    auto program = P4::parseP4String(getMultiVisitLoopSource());
    ASSERT_TRUE(program != nullptr);

    program = program->apply(MultiVisitModifier());
    ASSERT_TRUE(program != nullptr);
}

#if !HAVE_LIBGC
namespace {
class TrackedVisitorConstant final : public IR::Constant {
    int &live;

 public:
    TrackedVisitorConstant(int &live, int value) : IR::Constant(value), live(live) { ++live; }
    TrackedVisitorConstant(const TrackedVisitorConstant &other)
        : IR::Constant(other), live(other.live) {
        ++live;
    }
    ~TrackedVisitorConstant() override { --live; }
    TrackedVisitorConstant *clone() const override { return new TrackedVisitorConstant(*this); }
};

class ReplaceTrackedConstant : public Transform {
    int &live;

 public:
    explicit ReplaceTrackedConstant(int &live) : live(live) {}
    const IR::Node *preorder(IR::Constant *constant) override {
        return constant->value == 0 ? new TrackedVisitorConstant(live, 1) : constant;
    }
};
}  // namespace

TEST_F(P4CVisitor, PreorderReplacementReleasesExtraClone) {
    int live = 0;
    {
        IR::Ptr<IR::Constant> original = new TrackedVisitorConstant(live, 0);
        ReplaceTrackedConstant transform(live);
        auto result = original->apply(transform);
        EXPECT_EQ(result->checkedTo<IR::Constant>()->value, 1);
        EXPECT_EQ(original->value, 0);
        EXPECT_EQ(live, 2);
    }
    EXPECT_EQ(live, 0);
}

TEST_F(P4CVisitor, ControlFlowClonesReleaseSharedAnalysisState) {
    struct TrackedFlow : Inspector, ControlFlowVisitor {
        int &live;
        explicit TrackedFlow(int &live) : live(live) { ++live; }
        TrackedFlow(const TrackedFlow &other)
            : Visitor(other), Inspector(other), ControlFlowVisitor(other), live(other.live) {
            ++live;
        }
        ~TrackedFlow() override { --live; }
        TrackedFlow *clone() const override { return new TrackedFlow(*this); }
        void flow_merge(Visitor &) override {}
        void flow_copy(ControlFlowVisitor &) override {}
    };
    int live = 0;
    {
        TrackedFlow flow(live);
        flow.flow_merge_global_to("saved"_cs);
        EXPECT_EQ(live, 2);
        {
            ControlFlowVisitor::SaveGlobal save(flow, "saved"_cs);
            flow.flow_merge_global_to("saved"_cs);
            EXPECT_EQ(live, 3);
        }
        EXPECT_EQ(live, 2);
        flow.clear_globals();
        EXPECT_EQ(live, 1);
        IR::Ptr<IR::IfStatement> branch = new IR::IfStatement(
            new IR::BoolLiteral(true), new IR::EmptyStatement(), new IR::EmptyStatement());
        branch->apply(flow);
        EXPECT_EQ(live, 1);
        flow.flow_merge_global_to("retained-until-destruction"_cs);
    }
    EXPECT_EQ(live, 0);
}

TEST_F(P4CVisitor, PassManagersShareOwnedPassesAndBorrowStackPasses) {
    struct TrackedPass : Inspector {
        int &live;
        explicit TrackedPass(int &live) : live(live) {
            ++live;
            setName("TrackedPass");
        }
        ~TrackedPass() override { --live; }
    };
    int live = 0;
    {
        TrackedPass stackPass(live);
        {
            PassManager first({new TrackedPass(live), &stackPass});
            {
                PassManager second(first);
                first.removePasses({"TrackedPass"_cs});
                EXPECT_EQ(live, 2);
            }
            EXPECT_EQ(live, 1);
        }
        EXPECT_EQ(live, 1);
    }
    EXPECT_EQ(live, 0);
}
#endif

}  // namespace P4::Test
