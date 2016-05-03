#ifndef _MIDEND_LOCALIZEACTIONS_H_
#define _MIDEND_LOCALIZEACTIONS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"

namespace P4 {

class ActionReplacements {
 public:
    // For each global action and each control where the action is used
    // we create a replacement.
    std::map<const IR::P4Action*, std::map<const IR::P4Control*, const IR::P4Action*>*> repl;
    CallGraph<const IR::P4Action*> calls;

    bool isGlobal(const IR::P4Action* action) const
    { return repl.find(action) != repl.end(); }
    bool hasReplacement(const IR::P4Action* action, const IR::P4Control* control) const {
        auto map = ::get(repl, action);
        return ::get(map, control) != nullptr;
    }
    void addReplacement(const IR::P4Action* action, const IR::P4Control* control,
                        const IR::P4Action* replacement) {
        (*repl[action])[control] = replacement;
    }
};

class FindActionUses : public Inspector {
    ReferenceMap*       refMap;
    ActionReplacements* repl;
 public:
    FindActionUses(ReferenceMap* refMap, ActionReplacements* repl) : refMap(refMap), repl(repl)
    { CHECK_NULL(refMap); CHECK_NULL(repl); }
    // Global actions will be visited prior to their uses
    bool preorder(const IR::P4Action* action) override {
        if (findContext<IR::P4Control>() != nullptr)
            repl->repl[action] = new std::map<const IR::P4Control*, const IR::P4Action*>();
        return false;
    }
    bool preorder(const IR::PathExpression* path) override;
};

// Global actions are cloned into actions local to the
// control using them.  One action can produce many copies.
class LocalizeActions : public Transform {
    ReferenceMap*       refMap;
    ActionReplacements* repl;
 public:
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
};

}  // namespace P4

#endif /* _MIDEND_LOCALIZEACTIONS_H_ */
