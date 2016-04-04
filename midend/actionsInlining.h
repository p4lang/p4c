#ifndef _MIDEND_ACTIONSINLINING_H_
#define _MIDEND_ACTIONSINLINING_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/blockMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

struct ActionCallInfo {
    const IR::ActionContainer* caller;
    const IR::ActionContainer* callee;
    const IR::MethodCallStatement* call;

    ActionCallInfo(const IR::ActionContainer* caller, const IR::ActionContainer* callee,
                   const IR::MethodCallStatement* call) :
            caller(caller), callee(callee), call(call)
    { CHECK_NULL(caller); CHECK_NULL(callee); CHECK_NULL(call); }
    void dbprint(std::ostream& out) const
    { out << callee << " into " << caller << " at " << call; }
};

struct InlineWorkList {
    // Map caller -> statement -> callee
    std::map<const IR::ActionContainer*,
             std::map<const IR::MethodCallStatement*, const IR::ActionContainer*>> sites;
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
    InlineWorkList* next();
    void add(ActionCallInfo* aci)
    { toInline.push_back(aci); }

    void replace(const IR::ActionContainer* container, const IR::ActionContainer* replacement) {
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
            toInline(toInline), refMap(refMap), typeMap(typeMap) {}
    bool preorder(const IR::ParserContainer*) override { return false; }  // skip
    void postorder(const IR::MethodCallStatement* mcs) override;
};

class InlineActionsDriver : public Transform {
    ActionsInlineList* toInline;
    P4::ReferenceMap*  refMap;
    bool isv1;
 public:
    InlineActionsDriver(ActionsInlineList* toInline, P4::ReferenceMap* refMap, bool isv1) :
            toInline(toInline), refMap(refMap), isv1(isv1) {}

    // Not really a visitor, but we want to embed it into a PassManager,
    // so we make it look like a visitor.
    const IR::Node* preorder(IR::P4Program* program) override;
};

}  // namespace P4

namespace P4v1 {

class InlineActions : public Transform {
    const IR::Global    *global;
    class SubstActionArgs : public Transform {
        const IR::ActionFunction *function;
        const IR::Primitive *callsite;
        const IR::Expression *postorder(IR::ActionArg *arg) {
            for (unsigned i = 0; i < function->args.size(); ++i)
                if (function->args[i] == getOriginal())
                    return callsite->operands[i];
            BUG("Action arg not argument of action");
            return arg; }
     public:
        SubstActionArgs(const IR::ActionFunction *f, const IR::Primitive *c)
        : function(f), callsite(c) {}
    };
    const IR::Global *preorder(IR::Global *gl) { return global = gl; }
    const IR::Node *preorder(IR::Primitive *p) {
        if (auto af = global->get<IR::ActionFunction>(p->name))
            return af->action.clone()->apply(SubstActionArgs(af, p));
        return p; }
};

}  // namespace P4v1

#endif /* _MIDEND_ACTIONSINLINING_H_ */
