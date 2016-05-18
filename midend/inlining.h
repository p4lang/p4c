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

#ifndef _MIDEND_INLINING_H_
#define _MIDEND_INLINING_H_

#include "lib/ordered_map.h"
#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"

// These are various data structures needed by the parser/parser and control/control inliners.

namespace P4 {

// Describes information about a caller-callee pair
struct CallInfo {
    const IR::IContainer* caller;
    const IR::IContainer* callee;
    const IR::Declaration_Instance* instantiation;  // callee instantiation
    // Each instantiation may be invoked multiple times.
    std::set<const IR::MethodCallStatement*> invocations;  // all invocations within the caller

    CallInfo(const IR::IContainer* caller, const IR::IContainer* callee,
             const IR::Declaration_Instance* instantiation) :
            caller(caller), callee(callee), instantiation(instantiation)
    { CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation); }
    void addInvocation(const IR::MethodCallStatement* statement)
    { invocations.emplace(statement); }
    void dbprint(std::ostream& out) const
    { out << "Inline " << callee << " into " << caller; }
};

// Summarizes all inline operations to be performed.
struct InlineSummary {
    struct PerCaller {  // information for each caller
        // For each key instance the container that is intantiated.
        std::map<const IR::Declaration_Instance*, const IR::IContainer*> declToCallee;
        // For each key instance the apply parameters are replaced with fresh variables.
        std::map<const IR::Declaration_Instance*, std::map<cstring, cstring>*> paramRename;
        // For each key call the instance that is invoked.
        std::map<const IR::MethodCallStatement*, const IR::Declaration_Instance*> callToinstance;
    };
    std::map<const IR::IContainer*, PerCaller> callerToWork;

    void add(const CallInfo *cci) {
        callerToWork[cci->caller].declToCallee[cci->instantiation] = cci->callee;
        for (auto mcs : cci->invocations)
            callerToWork[cci->caller].callToinstance[mcs] = cci->instantiation;
    }
    void dbprint(std::ostream& out) const
    { out << "Inline " << callerToWork.size() << " call sites"; }
};

// Inling information constructed here.
class InlineWorkList {
    // We use an ordered map to make the iterator deterministic
    ordered_map<const IR::Declaration_Instance*, CallInfo*> inlineMap;
    std::vector<CallInfo*> toInline;  // sorted in order of inlining

 public:
    void addInstantiation(const IR::IContainer* caller, const IR::IContainer* callee,
                          const IR::Declaration_Instance* instantiation) {
        CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation);
        LOG1("Inline instantiation " << instantiation);
        auto inst = new CallInfo(caller, callee, instantiation);
        inlineMap[instantiation] = inst;
    }

    void addInvocation(const IR::Declaration_Instance* instance,
                       const IR::MethodCallStatement* statement) {
        CHECK_NULL(instance); CHECK_NULL(statement);
        LOG1("Inline invocation " << instance);
        auto info = inlineMap[instance];
        BUG_CHECK(info, "Could not locate instance %1% invoked by %2%", instance, statement);
        info->addInvocation(statement);
    }

    void replace(const IR::IContainer* container, const IR::IContainer* replacement) {
        CHECK_NULL(container); CHECK_NULL(replacement);
        LOG1("Replacing " << container << " with " << replacement);
        for (auto e : toInline) {
            if (e->callee == container)
                e->callee = replacement;
            if (e->caller == container)
                e->caller = replacement;
        }
    }

    void analyze(bool allowMultipleCalls);
    InlineSummary* next();
};

// Base class for an inliner:
// Information to inline is in list
// Future inlining information is in toInline; must be updated
// as inlining is performed (since callers change into new nodes).
class AbstractInliner : public Transform {
 protected:
    InlineWorkList* list;
    InlineSummary*  toInline;
    bool                p4v1;
    AbstractInliner() : list(nullptr), toInline(nullptr), p4v1(false) {}
 public:
    void prepare(InlineWorkList* list, InlineSummary* toInline, bool p4v1) {
        CHECK_NULL(list); CHECK_NULL(toInline);
        this->list = list;
        this->toInline = toInline;
        this->p4v1 = p4v1;
    }

    Visitor::profile_t init_apply(const IR::Node* node) {
        LOG1("AbstractInliner " << toInline);
        return Transform::init_apply(node);
    }
    virtual ~AbstractInliner() {}
};

// Repeatedly invokes an abstract inliner with work from the worklist
class InlineDriver : public Transform {
    InlineWorkList*  toInline;
    AbstractInliner*     inliner;
    bool                 p4v1;
 public:
    explicit InlineDriver(InlineWorkList* toInline, AbstractInliner* inliner, bool p4v1) :
            toInline(toInline), inliner(inliner), p4v1(p4v1)
    { CHECK_NULL(toInline); CHECK_NULL(inliner); setName("InlineDriver"); }

    // Not really a visitor, but we want to embed it into a PassManager,
    // so we make it look like a visitor.
    const IR::Node* preorder(IR::P4Program* program) override;
};

// Must be run after an evaluator; uses the blocks to discover caller/callee relationships.
class DiscoverInlining : public Inspector {
    InlineWorkList* inlineList;   // output: result is here
    ReferenceMap*   refMap;       // input
    TypeMap*        typeMap;      // input
    P4::Evaluator*  evaluator;    // used to obtain the toplevel block
    IR::ToplevelBlock* toplevel;

 public:
    bool allowParsers = true;
    bool allowControls = true;
    // The following cannot be inlined, but we can report errors if we detect them
    bool allowParsersFromControls = false;
    bool allowControlsFromParsers = false;

    DiscoverInlining(InlineWorkList* inlineList, ReferenceMap* refMap,
                     TypeMap* typeMap, P4::Evaluator* evaluator) :
            inlineList(inlineList), refMap(refMap), typeMap(typeMap),
            evaluator(evaluator), toplevel(nullptr) {
        CHECK_NULL(inlineList); CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(evaluator);
        setName("DiscoverInlining");
    }
    Visitor::profile_t init_apply(const IR::Node* node) override {
        toplevel = evaluator->getToplevelBlock();
        CHECK_NULL(toplevel);
        return Inspector::init_apply(node); }
    void visit_all(const IR::Block* block);
    bool preorder(const IR::Block* block) override
    { visit_all(block); return false; }
    bool preorder(const IR::ControlBlock* block) override;
    bool preorder(const IR::ParserBlock* block) override;
    void postorder(const IR::MethodCallStatement* statement) override;
    // We don't care to visit the program, we just visit the blocks.
    bool preorder(const IR::P4Program*) override
    { visit_all(toplevel); return false; }
};

// Performs actual inlining work
class GeneralInliner : public AbstractInliner {
    ReferenceMap* refMap;
    InlineSummary::PerCaller* workToDo;
 public:
    GeneralInliner() : refMap(new ReferenceMap()), workToDo(nullptr)
    { setName("GeneralInliner"); }
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::P4Control* caller) override;
    const IR::Node* preorder(IR::P4Parser* caller) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

}  // namespace P4

#endif /* _MIDEND_INLINING_H_ */
