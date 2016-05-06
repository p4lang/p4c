#ifndef _MIDEND_LOCALIZEACTIONS_H_
#define _MIDEND_LOCALIZEACTIONS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4 {

class GlobalActionReplacements {
 public:
    // For each control that uses an action and for each each global action
    // we create a replacement.
    std::map<const IR::P4Control*, ordered_map<const IR::P4Action*, const IR::P4Action*>*> repl;

    const IR::P4Action* getReplacement(const IR::P4Action* action,
                                       const IR::P4Control* control) const {
        auto map = ::get(repl, control);
        if (map == nullptr)
            return nullptr;
        if (map->find(action) != map->end())
            return (*map)[action];
        return nullptr;
    }
    void addReplacement(const IR::P4Action* action, const IR::P4Control* control,
                        const IR::P4Action* replacement) {
        LOG1("Cloning global " << action << " into " << replacement << " for " << control);
        if (repl.find(control) == repl.end())
            repl[control] = new ordered_map<const IR::P4Action*, const IR::P4Action*>();
        (*repl[control])[action] = replacement;
    }
};

// Find global (i.e., declared at toplevel) actions and who uses them.
class FindGlobalActionUses : public Inspector {
    ReferenceMap*       refMap;
    GlobalActionReplacements* repl;
    std::set<const IR::P4Action*> globalActions;
 public:
    FindGlobalActionUses(ReferenceMap* refMap, GlobalActionReplacements* repl)
    : refMap(refMap), repl(repl) { CHECK_NULL(refMap); CHECK_NULL(repl); }
    bool preorder(const IR::PathExpression* path) override;
    bool preorder(const IR::P4Action* action) override;
};

// Global actions are cloned into actions local to the
// control using them.  One action can produce many copies.
class LocalizeActions : public Transform {
    ReferenceMap*       refMap;
    GlobalActionReplacements* repl;
 public:
    LocalizeActions(ReferenceMap* refMap, GlobalActionReplacements* repl)
    : refMap(refMap), repl(repl) { CHECK_NULL(refMap); CHECK_NULL(repl); }
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
};

class LocalizeAllActions : public PassManager {
    GlobalActionReplacements replacements;
 public:
    LocalizeAllActions(ReferenceMap* refMap, bool isv1) {
        passes.emplace_back(new PassRepeated {
            new ResolveReferences(refMap, isv1),
            new FindGlobalActionUses(refMap, &replacements),
            new LocalizeActions(refMap, &replacements),
        });
        setName("LocalizeAllActions");
    }
};

}  // namespace P4

#endif /* _MIDEND_LOCALIZEACTIONS_H_ */
