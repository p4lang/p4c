#include "localizeActions.h"
#include "frontends/p4/cloner.h"

namespace P4 {

bool FindGlobalActionUses::preorder(const IR::P4Action* action) {
    if (findContext<IR::P4Control>() == nullptr)
        globalActions.emplace(action);
    return false;
}

bool FindGlobalActionUses::preorder(const IR::PathExpression* path) {
    auto decl = refMap->getDeclaration(path->path, true);
    if (!decl->is<IR::P4Action>())
        return false;

    auto action = decl->to<IR::P4Action>();
    if (globalActions.find(action) == globalActions.end())
        return false;
    auto control = findContext<IR::P4Control>();
    if (control != nullptr) {
        if (repl->getReplacement(action, control) != nullptr)
            return false;
        auto newName = refMap->newName(action->name);
        Cloner cloner;
        auto replBody = cloner.clone(action->body);
        BUG_CHECK(replBody->is<IR::Vector<IR::StatOrDecl>>(), "%1%: unexpected result", replBody);

        auto annos = action->annotations;
        if (annos == nullptr)
            annos = IR::Annotations::empty;
        annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                  new IR::StringLiteral(Util::SourceInfo(), action->name));
        auto replacement = new IR::P4Action(action->srcInfo, IR::ID(action->name.srcInfo, newName),
                                            annos, action->parameters,
                                            replBody->to<IR::IndexedVector<IR::StatOrDecl>>());
        repl->addReplacement(action, control, replacement);
    }
    return false;
}

const IR::Node* LocalizeActions::postorder(IR::P4Control* control) {
    auto actions = ::get(repl->repl, getOriginal<IR::P4Control>());
    if (actions == nullptr)
        return control;
    auto newDecls = new IR::IndexedVector<IR::Declaration>();
    for (auto pair : *actions) {
        auto toInsert = pair.second;
        LOG1("Adding " << toInsert);
        newDecls->push_back(toInsert);
    }

    for (auto d : *control->stateful)
        newDecls->push_back(d);
    control->stateful = newDecls;
    return control;
}

const IR::Node* LocalizeActions::postorder(IR::PathExpression* expression) {
    auto control = findOrigCtxt<IR::P4Control>();
    if (control == nullptr)
        return expression;
    auto decl = refMap->getDeclaration(expression->path);
    if (!decl->is<IR::P4Action>())
        return expression;
    auto action = decl->to<IR::P4Action>();
    auto replacement = repl->getReplacement(action, control);
    if (replacement != nullptr) {
        LOG1("Rewriting " << expression << " to " << replacement);
        expression = new IR::PathExpression(IR::ID(expression->srcInfo, replacement->name));
    }
    return expression;
}

}  // namespace P4
