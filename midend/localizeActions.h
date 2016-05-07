#ifndef _MIDEND_LOCALIZEACTIONS_H_
#define _MIDEND_LOCALIZEACTIONS_H_

#include "lib/ordered_set.h"
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

class ActionReplacement {
 public:
    // Where is this action used
    // Node is either a P4Table or MethodCallExpression
    std::map<const IR::P4Action*, std::set<const IR::Node*>> users;
    std::map<const IR::PathExpression*, const IR::P4Action*> repl;
    // For each action all replacements to insert
    std::map<const IR::P4Action*, ordered_set<const IR::P4Action*>*> toInsert;

    // Return true if this is a new user and not the first new user
    bool addActionUser(const IR::P4Action* action, const IR::Node* user) {
        bool isFirst = users.find(action) == users.end();
        if (users[action].find(user) == users[action].end()) {
            // new user
            users[action].emplace(user);
            return !isFirst;
        }
        // known user
        return false;
    }
    // For 'user' in control use the replacement action instead of the original action.
    void addReplacement(const IR::PathExpression* user,
                        const IR::P4Action* original,
                        const IR::P4Action* replacement) {
        LOG1("Adding replacement " << replacement << " used by " << user << " for " << original);
        repl[user] = replacement;
        if (toInsert.find(original) == toInsert.end())
            toInsert[original] = new ordered_set<const IR::P4Action*>();
        toInsert[original]->emplace(replacement);
    }
};

// Find actions that are invoked in multiple places; create a new
// copy for each invocation and store it in the repl map.  Ignores
// actions that are not in a control.
class FindRepeatedActionUses : public Inspector {
    ReferenceMap* refMap;
    ActionReplacement* repl;
 public:
    FindRepeatedActionUses(ReferenceMap* refMap, ActionReplacement* repl)
            : refMap(refMap), repl(repl) { CHECK_NULL(refMap); CHECK_NULL(repl); }
    bool preorder(const IR::PathExpression* expression) override;
};

// Replicates actions for each different user.
// Should be run after LocalizeActions.
class DuplicateActions : public Transform {
    ActionReplacement* repl;
 public:
    DuplicateActions(ActionReplacement* repl)
            : repl(repl) { CHECK_NULL(repl); }
    const IR::Node* postorder(IR::PathExpression* expression) override;
    const IR::Node* postorder(IR::P4Control* control) override;
};

class LocalizeAllActions : public PassManager {
    GlobalActionReplacements globalReplacements;
    ActionReplacement        localReplacements;
 public:
    LocalizeAllActions(ReferenceMap* refMap, bool isv1) {
        passes.emplace_back(new PassRepeated {
            new ResolveReferences(refMap, isv1),
            new FindGlobalActionUses(refMap, &globalReplacements),
            new LocalizeActions(refMap, &globalReplacements),
        });
        passes.emplace_back(new ResolveReferences(refMap, isv1));
        passes.emplace_back(new FindRepeatedActionUses(refMap, &localReplacements));
        passes.emplace_back(new DuplicateActions(&localReplacements));
        setName("LocalizeAllActions");
    }
};

}  // namespace P4

#endif /* _MIDEND_LOCALIZEACTIONS_H_ */
