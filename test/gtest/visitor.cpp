/*
Copyright 2024 Intel Corp.

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
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "midend_pass.h"

namespace Test {

using P4TestContext = P4CContextWithOptions<CompilerOptions>;

class P4CVisitor : public P4CTest {};

struct MultiVisitInspector : public Inspector, virtual public P4::ResolutionContext {
    MultiVisitInspector() { visitDagOnce = false; }

 private:
    // ignore parser and declaration loops for now
    void loop_revisit(const IR::ParserState *) override {}

    void visit_def(const IR::PathExpression *pe) {
        auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
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

    bool preorder(const IR::PathExpression *path) {
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
        auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
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

    bool preorder(IR::PathExpression *path) {
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
    auto *program =
        P4::parseP4String(getMultiVisitLoopSource(), CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);

    program = program->apply(MultiVisitInspector());
    ASSERT_TRUE(program != nullptr);
}

// This test fails when Visitor::ChangeTracker::try_start does _not_ reset visit_in_progress on a
// previously-visited node
TEST_F(P4CVisitor, MultiVisitModifierLoop) {
    auto *program =
        P4::parseP4String(getMultiVisitLoopSource(), CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(program != nullptr);

    program = program->apply(MultiVisitModifier());
    ASSERT_TRUE(program != nullptr);
}

}  // namespace Test
