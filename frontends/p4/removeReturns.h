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

#ifndef _FRONTENDS_P4_REMOVERETURNS_H_
#define _FRONTENDS_P4_REMOVERETURNS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

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

    void postorder(const IR::ExitStatement*) override
    { hasExits = true; }
    void postorder(const IR::ReturnStatement*) override
    { hasReturns = true; }
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
    P4::ReferenceMap* refMap;
    IR::ID            returnVar;  // one for each context
    IR::ID            returnedValue;  // only for functions that return expressions
    cstring           variableName;
    cstring           retValName;

    enum class Returns {
        Yes,
        No,
        Maybe
    };

    std::vector<Returns> stack;
    void push() { stack.push_back(Returns::No); }
    void pop() { stack.pop_back(); }
    void set(Returns r) { BUG_CHECK(!stack.empty(), "Empty stack"); stack.back() = r; }
    Returns hasReturned() { BUG_CHECK(!stack.empty(), "Empty stack"); return stack.back(); }

 public:
    explicit DoRemoveReturns(P4::ReferenceMap* refMap,
                             cstring varName = "hasReturned",
                             cstring retValName = "retval") :
            refMap(refMap), variableName(varName), retValName(retValName)
    { visitDagOnce = false; CHECK_NULL(refMap); setName("DoRemoveReturns"); }

    const IR::Node* preorder(IR::Function* function) override;
    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::ReturnStatement* statement) override;
    const IR::Node* preorder(IR::ExitStatement* statement) override;
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::SwitchStatement* statement) override;

    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
};

class RemoveReturns : public PassManager {
 public:
    explicit RemoveReturns(ReferenceMap* refMap) {
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new DoRemoveReturns(refMap));
        setName("RemoveReturns");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_REMOVERETURNS_H_ */
