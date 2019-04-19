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

#ifndef _FRONTENDS_P4_INLINING_H_
#define _FRONTENDS_P4_INLINING_H_

#include "lib/ordered_map.h"
#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/unusedDeclarations.h"
#include "commonInlining.h"

// These are various data structures needed by the parser/parser and control/control inliners.
// This only works correctly after local variable initializers have been removed,
// and after the SideEffectOrdering pass has been executed.

namespace P4 {

/// Describes information about a caller-callee pair
struct CallInfo : public IHasDbPrint {
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
    { out << "Inline " << callee << " into " << caller <<
                " with " << invocations.size() << " invocations"; }
};

class SymRenameMap {
    std::map<const IR::IDeclaration*, cstring> internalName;
    std::map<const IR::IDeclaration*, cstring> externalName;

 public:
    void setNewName(const IR::IDeclaration* decl, cstring name, cstring extName) {
        CHECK_NULL(decl);
        BUG_CHECK(!name.isNullOrEmpty() && !extName.isNullOrEmpty(), "Empty name");
        LOG3("setNewName " << dbp(decl) << " to " << name);
        if (internalName.find(decl) != internalName.end())
            BUG("%1%: already renamed", decl);
        internalName.emplace(decl, name);
        externalName.emplace(decl, extName);
    }
    cstring getName(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(internalName.find(decl) != internalName.end(), "%1%: no new name", decl);
        auto result = ::get(internalName, decl);
        return result;
    }
    cstring getExtName(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(externalName.find(decl) != externalName.end(), "%1%: no external name", decl);
        auto result = ::get(externalName, decl);
        return result;
    }
    bool isRenamed(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        return internalName.find(decl) != internalName.end();
    }
};

struct PerInstanceSubstitutions {
    ParameterSubstitution paramSubst;
    TypeVariableSubstitution tvs;
    SymRenameMap renameMap;
    PerInstanceSubstitutions() = default;
    PerInstanceSubstitutions(const PerInstanceSubstitutions &other) :
            paramSubst(other.paramSubst),
            tvs(other.tvs),
            renameMap(other.renameMap) {}
    template<class T>
    const T* rename(ReferenceMap* refMap, const IR::Node* node);
};

/// Summarizes all inline operations to be performed.
struct InlineSummary : public IHasDbPrint {
    /// Various substitutions that must be applied for each instance
    struct PerCaller {  // information for each caller
        /// For each instance (key) the container that is intantiated.
        std::map<const IR::Declaration_Instance*, const IR::IContainer*> declToCallee;
        /// For each instance (key) we must apply a bunch of substitutions
        std::map<const IR::Declaration_Instance*, PerInstanceSubstitutions*> substitutions;
        /// For each invocation (key) call the instance that is invoked.
        std::map<const IR::MethodCallStatement*, const IR::Declaration_Instance*> callToInstance;
        /// @returns nullptr if there isn't exactly one caller,
        /// otherwise the single caller of this instance.
        const IR::MethodCallStatement* uniqueCaller(
            const IR::Declaration_Instance* instance) const {
            const IR::MethodCallStatement* call = nullptr;
            for (auto m : callToInstance) {
                if (m.second == instance) {
                    if (call == nullptr)
                        call = m.first;
                    else
                        return nullptr;
                }
            }
            return call;
        }
    };
    std::map<const IR::IContainer*, PerCaller> callerToWork;

    void add(const CallInfo *cci) {
        callerToWork[cci->caller].declToCallee[cci->instantiation] = cci->callee;
        for (auto mcs : cci->invocations)
            callerToWork[cci->caller].callToInstance[mcs] = cci->instantiation;
    }
    void dbprint(std::ostream& out) const
    { out << "Inline " << callerToWork.size() << " call sites"; }
};

// Inling information constructed here.
class InlineList {
    // We use an ordered map to make the iterator deterministic
    ordered_map<const IR::Declaration_Instance*, CallInfo*> inlineMap;
    std::vector<CallInfo*> toInline;  // sorted in order of inlining
    const bool allowMultipleCalls = true;

 public:
    void addInstantiation(const IR::IContainer* caller, const IR::IContainer* callee,
                          const IR::Declaration_Instance* instantiation) {
        CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation);
        LOG3("Inline instantiation " << dbp(instantiation));
        auto inst = new CallInfo(caller, callee, instantiation);
        inlineMap[instantiation] = inst;
    }

    size_t size() const {
        return inlineMap.size();
    }

    void addInvocation(const IR::Declaration_Instance* instance,
                       const IR::MethodCallStatement* statement) {
        CHECK_NULL(instance); CHECK_NULL(statement);
        LOG3("Inline invocation " << dbp(instance) << " at " << dbp(statement));
        auto info = inlineMap[instance];
        BUG_CHECK(info, "Could not locate instance %1% invoked by %2%", instance, statement);
        info->addInvocation(statement);
    }

    void replace(const IR::IContainer* container, const IR::IContainer* replacement) {
        CHECK_NULL(container); CHECK_NULL(replacement);
        LOG3("Replacing " << dbp(container) << " with " << dbp(replacement));
        for (auto e : toInline) {
            if (e->callee == container)
                e->callee = replacement;
            if (e->caller == container)
                e->caller = replacement;
        }
    }

    void analyze();
    InlineSummary* next();
};

/// Must be run after an evaluator; uses the blocks to discover caller/callee relationships.
class DiscoverInlining : public Inspector {
    InlineList*         inlineList;  // output: result is here
    ReferenceMap*       refMap;      // input
    TypeMap*            typeMap;     // input
    IHasBlock*          evaluator;   // used to obtain the toplevel block
    IR::ToplevelBlock*  toplevel;

 public:
    bool allowParsers = true;
    bool allowControls = true;

    DiscoverInlining(InlineList* inlineList, ReferenceMap* refMap,
                     TypeMap* typeMap, IHasBlock* evaluator) :
            inlineList(inlineList), refMap(refMap), typeMap(typeMap),
            evaluator(evaluator), toplevel(nullptr) {
        CHECK_NULL(inlineList); CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(evaluator);
        setName("DiscoverInlining"); visitDagOnce = false;
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

/// Performs actual inlining work
class GeneralInliner : public AbstractInliner<InlineList, InlineSummary> {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    InlineSummary::PerCaller* workToDo;
 public:
    explicit GeneralInliner(bool isv1) :
            refMap(new ReferenceMap()), typeMap(new TypeMap()), workToDo(nullptr) {
        setName("GeneralInliner");
        refMap->setIsV1(isv1);
        visitDagOnce = false;
    }
    // controlled visiting order
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    template<class T> void inline_subst(T *caller, IR::IndexedVector<IR::Declaration> T::*locals);
    const IR::Node* preorder(IR::P4Control* caller) override;
    const IR::Node* preorder(IR::P4Parser* caller) override;
    const IR::Node* preorder(IR::ParserState* state) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

/// Performs one round of inlining bottoms-up
class InlinePass : public PassManager {
    InlineList toInline;
 public:
    InlinePass(ReferenceMap* refMap, TypeMap* typeMap, EvaluatorPass* evaluator)
    : PassManager({
        new TypeChecking(refMap, typeMap),
        new DiscoverInlining(&toInline, refMap, typeMap, evaluator),
        new InlineDriver<InlineList, InlineSummary>(&toInline, new GeneralInliner(refMap->isV1())),
        new RemoveAllUnusedDeclarations(refMap) }) { }
};

/**
Performs inlining as many times as necessary.  Most frequently once
will be enough.  Multiple iterations are necessary only when instances are
passed as arguments using constructor arguments.
*/
class Inline : public PassRepeated {
 public:
    Inline(ReferenceMap* refMap, TypeMap* typeMap, EvaluatorPass* evaluator)
    : PassManager({
        new InlinePass(refMap, typeMap, evaluator),
        // After inlining the output of the evaluator changes, so
        // we have to run it again
        evaluator }) {}
};

}  // namespace P4

#endif /* _FRONTENDS_P4_INLINING_H_ */
