#include "localizeActions.h"
#include "frontends/p4/cloner.h"

namespace P4 {

bool FindActionUses::preorder(const IR::PathExpression* path) {
    auto decl = refMap->getDeclaration(path->path, true);
    if (!decl->is<IR::P4Action>())
        return false;

    auto action = decl->to<IR::P4Action>();
    auto control = findContext<IR::P4Control>();
    auto caller = findContext<IR::P4Action>();
    if (control != nullptr) {
        // We compute replacements for the current action AND all its callees
        std::vector<const IR::P4Action*> callees;
        bool cycles = repl->calls.sccSort(action, callees);
        BUG_CHECK(!cycles, "%1%: cycle in action call graph?", action);

        for (auto callee : callees) {
            if (repl->hasReplacement(callee, control))
                continue;
            auto newName = refMap->newName(callee->name);
            Cloner cloner;
            auto replBody = cloner.clone(callee->body);
            BUG_CHECK(replBody->is<IR::Vector<IR::StatOrDecl>>(), "%1%: unexpected result", replBody);

            auto annos = callee->annotations;
            if (annos == nullptr)
                annos = IR::Annotations::empty;
            annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                      new IR::StringLiteral(Util::SourceInfo(), callee->name));
            auto replacement = new IR::P4Action(callee->srcInfo, newName,
                                                annos, callee->parameters,
                                                replBody->to<IR::Vector<IR::StatOrDecl>>());
            repl->addReplacement(callee, control, replacement);
        }
        return false;
    } else if (caller != nullptr) {
        // If we are both in a control and in an action we don't need to add the action
        // to the call graph
        repl->calls.add(caller, action);
    }
    return false;
}

const IR::Node* LocalizeActions::postorder(IR::P4Control* control) {
    auto actions = ::get(repl->repl, control);
    if (actions == nullptr)
        return control;
    auto newDecls = new IR::NameMap<IR::Declaration, ordered_map>();
    for (auto pair : *actions) {
        auto toInsert = pair.second;
        newDecls->addUnique(toInsert->name, toInsert);
    }

    for (auto d : control->stateful)
        newDecls->addUnique(d.first, d.second);
    control->stateful = std::move(*newDecls);
    return control;
}

const IR::Node* LocalizeActions::postorder(IR::PathExpression* expression) {
    auto decl = refMap->getDeclaration(expression->path);
    if (!decl->is<IR::P4Action>())
        return expression;
    auto action = decl->to<IR::P4Action>();
    return expression;
}

}  // namespace P4
