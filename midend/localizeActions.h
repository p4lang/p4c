#ifndef _MIDEND_LOCALIZEACTIONS_H_
#define _MIDEND_LOCALIZEACTIONS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"

namespace P4 {

class ActionReplacements {
 public:
    // For each control that uses an action and for each each global action
    // we create a replacement.
    std::map<const IR::P4Control*, std::map<const IR::P4Action*, const IR::P4Action*>*> repl;
    CallGraph<const IR::P4Action*> calls;

    bool hasReplacement(const IR::P4Action* action, const IR::P4Control* control) const {
        auto map = ::get(repl, control);
        if (map == nullptr)
            return false;
        return ::get(map, action) != nullptr;
    }
    void addReplacement(const IR::P4Action* action, const IR::P4Control* control,
                        const IR::P4Action* replacement) {
        LOG1("Replacing global " << action << " with " << replacement);
        if (repl.find(control) == repl.end())
            repl[control] = new std::map<const IR::P4Action*, const IR::P4Action*>();
        (*repl[control])[action] = replacement;
    }
};

class FindActionUses : public Inspector {
    ReferenceMap*       refMap;
    ActionReplacements* repl;
 public:
    FindActionUses(ReferenceMap* refMap, ActionReplacements* repl) : refMap(refMap), repl(repl)
    { CHECK_NULL(refMap); CHECK_NULL(repl); }
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
