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

#ifndef FRONTENDS_P4_REMOVERETURNS_H_
#define FRONTENDS_P4_REMOVERETURNS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/ternaryBool.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {
using namespace literals;

/**
This inspector detects whether an IR tree contains
'return' or 'exit' statements.
It sets a boolean flag for each of them.
*/
class HasExits : public Inspector {
 public:
    bool hasExits;
    bool hasReturns;
    HasExits() : hasExits(false), hasReturns(false) { setName("HasExits"); }

    void postorder(const IR::ExitStatement *) override { hasExits = true; }
    void postorder(const IR::ReturnStatement *) override { hasReturns = true; }
};

/**
This visitor rewrites code after an 'if' that does a jump to somewhere else (with
a break/continue/exit/return statement in one branch) by moving the code after the
if into the branch that does not jump.  A block that looks like

    ..some code
    if (pred) jump somewhere;
    ..more code
becomes:
    ..some code
    if (pred) {
        jump somewhere;
    } else {
        ..more code

it also deals with cases where the else is not empty (the 'more code' is appended), or
the jump is in the else branch (adds code to the ifTrue branch).

The purpose of this is to rewrite code such the DoRemoveReturns (and similar rewrites
for other jump statements like exit or loop break/continue) will more likely not need to
introduce a boolean flag and extra tests to remove those branches.

precondition:
    switchAddDefault pass has run to ensure switch statements cover all cases
*/
class MoveToElseAfterBranch : public Modifier {
    /* This pass does not use (inherit from) ControlFlowVisitor, even though it is doing
     * control flow analysis, as it turns out to be more efficient to do it directly here
     * by overloading the branching constructs (if/switch/loops) and not cloning the visitor,
     * because we *only* care about statements (not expressions) locally in a block (and
     * not across calls), and we only have one bit of data (hasJumped flag). */

    /** Set to true after a jump, indicating the code point we're at is unreachable.
     * Set back to false at label-like points where code might jump to */
    bool hasJumped = false;

    /** Set to true when code in a parent BlockStatement has been moved into an if branch,
     * indicating that it needs to be removed from the BlockStatment */
    bool movedToIfBranch = false;

    bool preorder(IR::BlockStatement *) override;
    bool moveFromParentTo(const IR::Statement *&child);
    bool preorder(IR::IfStatement *) override;
    bool preorder(IR::SwitchStatement *) override;
    void postorder(IR::LoopStatement *) override;
    bool branch() {
        hasJumped = true;
        // no need to visit children
        return false;
    }
    bool preorder(IR::BreakStatement *) override { return branch(); }
    bool preorder(IR::ContinueStatement *) override { return branch(); }
    bool preorder(IR::ExitStatement *) override { return branch(); }
    bool preorder(IR::ReturnStatement *) override { return branch(); }
    // Only visit statements, skip all expressions
    bool preorder(IR::Expression *) override { return false; }

 public:
    MoveToElseAfterBranch() {}
};

/**
This visitor replaces 'returns' by ifs:
e.g.
control c(inout bit x) { apply {
   if (x) return;
   x = !x;
}}
becomes:
control c(inout bit x) {
   bool hasReturned;
   apply {
     hasReturned = false;
     if (x) hasReturned = true;
     if (!hasReturned)
        x = !x;
}}
*/
class DoRemoveReturns : public Transform {
 protected:
    MinimalNameGenerator nameGen;
    IR::ID returnVar;      // one for each context
    IR::ID returnedValue;  // only for functions that return expressions
    cstring variableName;
    cstring retValName;

    std::vector<TernaryBool> stack;
    void push() { stack.push_back(TernaryBool::No); }
    void pop() { stack.pop_back(); }
    void set(TernaryBool r) {
        BUG_CHECK(!stack.empty(), "Empty stack");
        stack.back() = r;
    }
    TernaryBool hasReturned() {
        BUG_CHECK(!stack.empty(), "Empty stack");
        return stack.back();
    }

 public:
    explicit DoRemoveReturns(cstring varName = "hasReturned"_cs, cstring retValName = "retval"_cs)
        : variableName(varName), retValName(retValName) {
        visitDagOnce = false;
        setName("DoRemoveReturns");
    }

    const IR::Node *preorder(IR::Function *function) override;
    const IR::Node *preorder(IR::BlockStatement *statement) override;
    const IR::Node *preorder(IR::ReturnStatement *statement) override;
    const IR::Node *preorder(IR::ExitStatement *statement) override;
    const IR::Node *preorder(IR::IfStatement *statement) override;
    const IR::Node *preorder(IR::SwitchStatement *statement) override;

    const IR::Node *preorder(IR::P4Action *action) override;
    const IR::Node *preorder(IR::P4Control *control) override;
    const IR::Node *preorder(IR::P4Parser *parser) override {
        prune();
        return parser;
    }

    const IR::Node *postorder(IR::LoopStatement *loop) override;
    profile_t init_apply(const IR::Node *node) override;
};

class RemoveReturns : public PassManager {
 public:
    RemoveReturns() : PassManager({new MoveToElseAfterBranch(), new DoRemoveReturns()}) {}
};

}  // namespace P4

#endif /* FRONTENDS_P4_REMOVERETURNS_H_ */
