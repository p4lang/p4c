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

namespace P4 {

struct ActionCallInfo {
    const IR::P4Action* caller;
    const IR::P4Action* callee;
    const IR::MethodCallStatement* call;

    ActionCallInfo(const IR::P4Action* caller, const IR::P4Action* callee,
                   const IR::MethodCallStatement* call) :
            caller(caller), callee(callee), call(call)
    { CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(call); }
    void dbprint(std::ostream& out) const
    { out << callee << " into " << caller << " at " << call; }
};

struct AInlineWorkList {
    // Map caller -> statement -> callee
    std::map<const IR::P4Action*,
             std::map<const IR::MethodCallStatement*, const IR::P4Action*>> sites;
    void add(ActionCallInfo* info) {
        CHECK_NULL(info);
        sites[info->caller][info->call] = info->callee;
    }
    void dbprint(std::ostream& out) const;
    bool empty() const
    { return sites.empty(); }
};

class ActionsInlineList {
    std::vector<ActionCallInfo*> toInline;     // initial data
    std::vector<ActionCallInfo*> inlineOrder;  // sorted in inlining order
 public:
    void analyze();  // generate the inlining order
    AInlineWorkList* next();
    void add(ActionCallInfo* aci)
    { toInline.push_back(aci); }

    void replace(const IR::P4Action* container, const IR::P4Action* replacement) {
        LOG1("Substituting " << container << " with " << replacement);
        for (auto e : inlineOrder) {
            if (e->callee == container)
                e->callee = replacement;
            if (e->caller == container)
                e->caller = replacement;
        }
    }
};

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

// Base class for an inliner that processes actions.
class AbstractActionInliner : public Transform {
 protected:
    ActionsInlineList* list;
    AInlineWorkList*   toInline;
    AbstractActionInliner() : list(nullptr), toInline(nullptr) {}
 public:
    void prepare(ActionsInlineList* list, AInlineWorkList* toInline) {
        CHECK_NULL(list); CHECK_NULL(toInline);
        this->list = list;
        this->toInline = toInline;
    }
    Visitor::profile_t init_apply(const IR::Node* node) {
        LOG1("AbstractActionInliner " << toInline);
        return Transform::init_apply(node);
    }
    virtual ~AbstractActionInliner() {}
};

// General-purpose actions inliner.
class ActionsInliner : public AbstractActionInliner {
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

class InlineActionsDriver : public Transform {
    ActionsInlineList*     toInline;
    AbstractActionInliner* inliner;
 public:
    InlineActionsDriver(ActionsInlineList* toInline, AbstractActionInliner *inliner) :
            toInline(toInline), inliner(inliner)
    { CHECK_NULL(toInline); CHECK_NULL(inliner); setName("InlineActionsDriver"); }
    // Not really a visitor, but we want to embed it into a PassManager,
    // so we make it look like a visitor.
    const IR::Node* preorder(IR::P4Program* program) override;
};

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

// Special inliner which works directly on P4 v1.0 representation
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
