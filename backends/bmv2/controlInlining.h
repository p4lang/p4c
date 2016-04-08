#ifndef _BACKENDS_BMV2_CONTROLINLINING_H_
#define _BACKENDS_BMV2_CONTROLINLINING_H_

#include "ir/ir.h"
#include "frontends/p4/evaluator/blockMap.h"

namespace BMV2 {

// An inliner that only works for programs imported from P4 v1.0

struct ControlsCallInfo {
    const IR::P4Control* caller;
    const IR::P4Control* callee;
    const IR::Declaration_Instance* instantiation;
    // each instantiation may be invoked multiple times
    std::set<const IR::MethodCallStatement*> invocations;

    ControlsCallInfo(const IR::P4Control* caller, const IR::P4Control* callee,
                       const IR::Declaration_Instance* instantiation) :
            caller(caller), callee(callee), instantiation(instantiation)
    { CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation); }
    void addInvocation(const IR::MethodCallStatement* statement)
    { invocations.emplace(statement); }
    void dbprint(std::ostream& out) const
    { out << "Inline " << callee << " into " << caller; }
};

struct CInlineWorkList {
    struct PerCaller {
        std::map<const IR::Declaration_Instance*, const IR::P4Control*> declToCallee;
        std::map<const IR::MethodCallStatement*, const IR::Declaration_Instance*> callToinstance;
    };
    std::map<const IR::P4Control*, PerCaller> callerToWork;

    void add(ControlsCallInfo* cci);
};

class ControlsInlineList {
    std::map<const IR::Declaration_Instance*, ControlsCallInfo*> inlineMap;
    std::vector<ControlsCallInfo*> toInline;  // sorted in order of inlining

 public:
    CInlineWorkList* next();
    void addInstantiation(const IR::P4Control* caller, const IR::P4Control* callee,
                          const IR::Declaration_Instance* instantiation) {
        CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation);
        LOG1("Inline instantiation " << instantiation);
        auto inst = new ControlsCallInfo(caller, callee, instantiation);
        inlineMap.emplace(instantiation, inst);
    }

    void addInvocation(const IR::Declaration_Instance* instance,
                       const IR::MethodCallStatement* statement) {
        CHECK_NULL(instance); CHECK_NULL(statement);
        LOG1("Inline invocation " << instance);
        auto info = ::get(inlineMap, instance);
        BUG_CHECK(info, "Could not locate instance %1% invoked by %2%", instance, statement);
        info->addInvocation(statement);
    }

    void replace(const IR::P4Control* container, const IR::P4Control* replacement) {
        CHECK_NULL(container); CHECK_NULL(replacement);
        LOG1("Replacing " << container << " with " << replacement);
        for (auto e : toInline) {
            if (e->callee == container)
                e->callee = replacement;
            if (e->caller == container)
                e->caller = replacement;
        }
    }

    void analyze();
};

class DiscoverControlsInlining : public Inspector {
    ControlsInlineList* inlineList;
    P4::BlockMap* blockMap;
 public:
    DiscoverControlsInlining(ControlsInlineList* inlineList, P4::BlockMap* blockMap) :
            inlineList(inlineList), blockMap(blockMap)
    { CHECK_NULL(inlineList); CHECK_NULL(blockMap); }
    void visit_all(const IR::Block* block);
    bool preorder(const IR::Block* block) override
    { visit_all(block); return false; }
    bool preorder(const IR::ControlBlock* block) override;
    bool preorder(const IR::ParserBlock* block) override;
    void postorder(const IR::MethodCallStatement* statement) override;
    // We don't care to visit the program, we just visit the blocks.
    bool preorder(const IR::P4Program*) override
    { visit_all(blockMap->toplevelBlock); return false; }
};

class InlineControlsDriver : public Transform {
    P4::ReferenceMap* refMap;
    ControlsInlineList* toInline;

 public:
    explicit InlineControlsDriver(P4::ReferenceMap* refMap, ControlsInlineList* toInline) :
            refMap(refMap), toInline(toInline)
    { CHECK_NULL(refMap); CHECK_NULL(toInline); }

    // Not really a visitor, but we want to embed it into a PassManager,
    // so we make it look like a visitor.
    const IR::Node* preorder(IR::P4Program* program) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_CONTROLINLINING_H_ */
