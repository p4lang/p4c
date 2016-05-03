#include "localizeActions.h"
#include "frontends/p4/cloner.h"

namespace P4 {

bool FindActionUses::preorder(const IR::PathExpression* path) {
    auto decl = refMap->getDeclaration(path->path, true);
    if (!decl->is<IR::P4Action>())
        return false;

    auto action = decl->to<IR::P4Action>();
    if (!repl->isGlobal(action))
        return false;
    auto control = findContext<IR::P4Control>();
    auto caller = findContext<IR::P4Action>();
    if (control != nullptr) {
        if (repl->hasReplacement(action, control))
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
        auto replacement = new IR::P4Action(action->srcInfo, newName,
                                            annos, action->parameters,
                                            replBody->to<IR::Vector<IR::StatOrDecl>>());
        repl->addReplacement(action, control, replacement);
        return false;
    }
    repl->calls.add(caller, action);
    // TODO
    return false;
}

const IR::Node* LocalizeActions::postorder(IR::P4Control* control) {
    // TODO
    return control;
}

const IR::Node* LocalizeActions::postorder(IR::PathExpression* expression) {
    // TODO
    return expression;
}

}  // namespace P4
