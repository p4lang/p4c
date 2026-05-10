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

#ifndef FRONTENDS_P4_ACTIONSINLINING_H_
#define FRONTENDS_P4_ACTIONSINLINING_H_

#include "commonInlining.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"

namespace P4 {

using ActionCallInfo = SimpleCallInfo<IR::P4Action, IR::MethodCallStatement>;
using AInlineWorkList = SimpleInlineWorkList<ActionCallInfo>;
using ActionsInlineList = SimpleInlineList<IR::P4Action, ActionCallInfo, AInlineWorkList>;

class DiscoverActionsInlining : public Inspector, public ResolutionContext {
    ActionsInlineList *toInline;  // output
    P4::TypeMap *typeMap;         // input
 public:
    DiscoverActionsInlining(ActionsInlineList *toInline, P4::TypeMap *typeMap)
        : toInline(toInline), typeMap(typeMap) {
        CHECK_NULL(toInline);
        CHECK_NULL(typeMap);
        setName("DiscoverActionsInlining");
    }
    bool preorder(const IR::P4Parser *) override { return false; }  // skip
    void postorder(const IR::MethodCallStatement *mcs) override;
};

// General-purpose actions inliner.
class ActionsInliner : public AbstractInliner<ActionsInlineList, AInlineWorkList> {
    std::unique_ptr<MinimalNameGenerator> nameGen;
    AInlineWorkList::ReplacementMap *replMap;

 public:
    ActionsInliner() : replMap(nullptr) {}
    Visitor::profile_t init_apply(const IR::Node *node) override;
    const IR::Node *preorder(IR::P4Parser *cont) override {
        prune();
        return cont;
    }  // skip
    const IR::Node *preorder(IR::P4Action *action) override;
    const IR::Node *postorder(IR::P4Action *action) override;
    const IR::Node *preorder(IR::MethodCallStatement *statement) override;
};

typedef InlineDriver<ActionsInlineList, AInlineWorkList> InlineActionsDriver;

class InlineActions : public PassManager {
    ActionsInlineList actionsToInline;

 public:
    InlineActions(TypeMap *typeMap, const RemoveUnusedPolicy &policy) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DiscoverActionsInlining(&actionsToInline, typeMap));
        passes.push_back(new InlineActionsDriver(&actionsToInline, new ActionsInliner()));
        passes.push_back(new RemoveAllUnusedDeclarations(policy));
        setName("InlineActions");
    }
};

}  // namespace P4

#ifdef SUPPORT_P4_14
#include "actionsInlining_p4_14.h"
#endif

#endif /* FRONTENDS_P4_ACTIONSINLINING_H_ */
