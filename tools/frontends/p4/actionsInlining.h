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

#ifndef _FRONTENDS_P4_ACTIONSINLINING_H_
#define _FRONTENDS_P4_ACTIONSINLINING_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "commonInlining.h"

namespace P4 {

typedef SimpleCallInfo<IR::P4Action, IR::MethodCallStatement> ActionCallInfo;
typedef SimpleInlineWorkList<IR::P4Action, IR::MethodCallStatement, ActionCallInfo> AInlineWorkList;
typedef SimpleInlineList<IR::P4Action, ActionCallInfo, AInlineWorkList> ActionsInlineList;

class DiscoverActionsInlining : public Inspector {
    ActionsInlineList* toInline;  // output
    P4::ReferenceMap*  refMap;    // input
    P4::TypeMap*       typeMap;   // input
 public:
    DiscoverActionsInlining(ActionsInlineList* toInline,
                            P4::ReferenceMap* refMap,
                            P4::TypeMap* typeMap) :
            toInline(toInline), refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(toInline); CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DiscoverActionsInlining"); }
    bool preorder(const IR::P4Parser*) override { return false; }  // skip
    void postorder(const IR::MethodCallStatement* mcs) override;
};

// General-purpose actions inliner.
class ActionsInliner : public AbstractInliner<ActionsInlineList, AInlineWorkList> {
    P4::ReferenceMap* refMap;
    std::map<const IR::MethodCallStatement*, const IR::P4Action*>* replMap;
 public:
    explicit ActionsInliner(bool isv1) : refMap(new P4::ReferenceMap()), replMap(nullptr)
    { refMap->setIsV1(isv1); }
    Visitor::profile_t init_apply(const IR::Node* node) override;
    const IR::Node* preorder(IR::P4Parser* cont) override
    { prune(); return cont; }  // skip
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
};

typedef InlineDriver<ActionsInlineList, AInlineWorkList> InlineActionsDriver;

class InlineActions : public PassManager {
    ActionsInlineList actionsToInline;
 public:
    InlineActions(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DiscoverActionsInlining(&actionsToInline, refMap, typeMap));
        passes.push_back(new InlineActionsDriver(&actionsToInline,
                                                 new ActionsInliner(refMap->isV1())));
        passes.push_back(new RemoveAllUnusedDeclarations(refMap));
        setName("InlineActions");
    }
};

}  // namespace P4

namespace P4_14 {

/// Special inliner which works directly on P4-14 representation
class InlineActions : public Transform {
    const IR::V1Program    *global;
    class SubstActionArgs : public Transform {
        const IR::ActionFunction *function;
        const IR::Primitive *callsite;
        const IR::Node *postorder(IR::ActionArg *arg) override {
            for (unsigned i = 0; i < function->args.size(); ++i)
                if (function->args[i] == getOriginal())
                    return callsite->operands[i];
            BUG("Action arg not argument of action");
            return arg; }
     public:
        SubstActionArgs(const IR::ActionFunction *f, const IR::Primitive *c)
        : function(f), callsite(c) {}
    };
    const IR::V1Program *preorder(IR::V1Program *gl) override { return global = gl; }
    const IR::Node *preorder(IR::Primitive *p) override {
        if (auto af = global->get<IR::ActionFunction>(p->name))
            return af->action.clone()->apply(SubstActionArgs(af, p));
        return p; }
};

}  // namespace P4_14

#endif /* _FRONTENDS_P4_ACTIONSINLINING_H_ */
