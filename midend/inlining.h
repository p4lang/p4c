#ifndef _MIDEND_INLINING_H_
#define _MIDEND_INLINING_H_

#include "ir/ir.h"
#include "frontends/p4/evaluator/blockMap.h"

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
        std::map<const IR::Declaration_Instance*, const IR::IContainer*> declToCallee;
        std::map<const IR::MethodCallStatement*, const IR::Declaration_Instance*> callToinstance;
    };
    std::map<const IR::IContainer*, PerCaller> callerToWork;

    void add(const CallInfo *cci) {
        callerToWork[cci->caller].declToCallee[cci->instantiation] = cci->callee;
        for (auto mcs : cci->invocations) {
            callerToWork[cci->caller].callToinstance[mcs] = cci->instantiation;
        }
    }
};

// Inling information constructed here.
class InlineWorkList {
    std::map<const IR::Declaration_Instance*, CallInfo*> inlineMap;
    std::vector<CallInfo*> toInline;  // sorted in order of inlining

 public:
    void addInstantiation(const IR::IContainer* caller, const IR::IContainer* callee,
                          const IR::Declaration_Instance* instantiation) {
        CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(instantiation);
        LOG1("Inline instantiation " << instantiation);
        auto inst = new CallInfo(caller, callee, instantiation);
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
    P4::InlineWorkList* list;
    P4::InlineSummary*  toInline;
    bool                p4v1;
    AbstractInliner() : list(nullptr), toInline(nullptr), p4v1(false) {}
 public:
    void prepare(P4::InlineWorkList* list, P4::InlineSummary* toInline, bool p4v1) {
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
    P4::InlineWorkList*  toInline;
    AbstractInliner*     inliner;
    bool                 p4v1;
    
 public:
    explicit InlineDriver(P4::InlineWorkList* toInline, AbstractInliner* inliner, bool p4v1) :
            toInline(toInline), inliner(inliner), p4v1(p4v1)
    { CHECK_NULL(toInline); CHECK_NULL(inliner); }

    // Not really a visitor, but we want to embed it into a PassManager,
    // so we make it look like a visitor.
    const IR::Node* preorder(IR::P4Program* program) override;
};

// Must be run after an evaluator; uses the blockMap to discover caller/callee relationships.
class DiscoverInlining : public Inspector {
    P4::InlineWorkList* inlineList;  // deposit result here
    P4::BlockMap* blockMap;
 public:
    bool allowParsers = true;
    bool allowControls = true;
    // The following cannot be inlined, but we can report errors if we detect them
    bool allowParsersFromControls = false;
    bool allowControlsFromParsers = false;

    DiscoverInlining(P4::InlineWorkList* inlineList, P4::BlockMap* blockMap) :
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

class GeneralInliner : public AbstractInliner {
    // TODO
};

}  // namespace P4

#endif /* _MIDEND_INLINING_H_ */
