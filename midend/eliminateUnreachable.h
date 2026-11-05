/*
SPDX-FileCopyrightText: 2026 Nvidia Corporation

SPDX-License-Identifier: Apache-2.0

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

#ifndef MIDEND_ELIMINATEUNREACHABLE_H_
#define MIDEND_ELIMINATEUNREACHABLE_H_

namespace P4 {

class ElimUnreachableStmts : public Transform {
 private:
    bool unreachable = false;

    // top-level things that are always reachable
    IR::Node *preorder(IR::Function *fn) {
        unreachable = false;
        return fn;
    }
    IR::Node *preorder(IR::Method *fn) {
        unreachable = false;
        return fn;
    }
    IR::Node *preorder(IR::P4Action *act) {
        unreachable = false;
        return act;
    }
    IR::Node *preorder(IR::P4Control *c) {
        unreachable = false;
        return c;
    }
    IR::Node *preorder(IR::P4Parser *p) {
        unreachable = false;
        return p;
    }
    IR::Node *preorder(IR::ParserState *s) {
        unreachable = false;
        return s;
    }

    // reset flag after declarations
    IR::Node *postorder(IR::Declaration *d) {
        unreachable = false;
        return d;
    }

    // prune visiting expressions as they can't contain statements
    IR::Node *postorder(IR::Expression *e) {
        prune();
        return e;
    }

    IR::Node *preorder(IR::Statement *s) {
        if (unreachable) return nullptr;
        return s;
    }
    IR::Node *preorder(IR::Declaration_Variable *v) {
        if (unreachable) return nullptr;
        return v;
    }
    IR::Node *preorder(IR::BreakStatement *s) {
        if (unreachable) return nullptr;
        unreachable = true;
        return s;
    }
    IR::Node *preorder(IR::ContinueStatement *s) {
        if (unreachable) return nullptr;
        unreachable = true;
        return s;
    }
    IR::Node *preorder(IR::ExitStatement *s) {
        if (unreachable) return nullptr;
        unreachable = true;
        return s;
    }
    IR::Node *preorder(IR::ReturnStatement *s) {
        if (unreachable) return nullptr;
        unreachable = true;
        return s;
    }
    IR::Node *preorder(IR::IfStatement *ifstmt) {
        if (unreachable) return nullptr;
        prune();
        visit(ifstmt->ifTrue, "ifTrue");
        bool jumps = unreachable;
        unreachable = false;
        visit(ifstmt->ifFalse, "ifTrue");
        unreachable &= jumps;
        if (!ifstmt->ifTrue) ifstmt->ifTrue = new IR::EmptyStatement;
        return ifstmt;
    }
    IR::Node *preorder(IR::SwitchStatement *swtch) {
        if (unreachable) return nullptr;
        prune();
        bool final_unreachable = true;
        bool has_default = false;
        for (auto &c : swtch->cases) {
            if (c->label->is<IR::DefaultExpression>()) has_default = true;
            if (c->statement) {
                unreachable = false;
                visit(c, "cases");
                BUG_CHECK(c->statement, "bogus eliminated case");
                final_unreachable &= unreachable;
            }
        }
        unreachable = final_unreachable & has_default;
        return swtch;
    }
    IR::Node *preorder(IR::ForStatement *loop) {
        if (unreachable) return nullptr;
        prune();
        visit(loop->init, "init");
        BUG_CHECK(!unreachable, "jump in loop init statements");
        visit(loop->body, "body");
        unreachable = false;
        visit(loop->updates, "updates");
        unreachable = false;
        if (!loop->body) loop->body = new IR::EmptyStatement;
        return loop;
    }
    IR::Node *preorder(IR::ForInStatement *loop) {
        if (unreachable) return nullptr;
        prune();
        visit(loop->body, "body");
        unreachable = false;
        if (!loop->body) loop->body = new IR::EmptyStatement;
        return loop;
    }

 public:
    explicit ElimUnreachableStmts() {}
};

}  // namespace P4

#endif /* MIDEND_ELIMINATEUNREACHABLE_H_ */
