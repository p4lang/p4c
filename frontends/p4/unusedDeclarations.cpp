#include "unusedDeclarations.h"
#include "simplify.h"

namespace P4 {

Visitor::profile_t RemoveUnusedDeclarations::init_apply(const IR::Node* node) {
    LOG2("Reference map " << refMap);
    return Transform::init_apply(node);
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Type_Enum* type) {
    prune();  // never remove individual enum members
    if (!refMap->isUsed(getOriginal<IR::Type_Enum>())) {
        LOG1("Removing " << type);
        return nullptr;
    }

    return type;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Control* cont) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        LOG1("Removing " << cont);
        prune();
        return nullptr;
    }

    cont->stateful.visit_children(*this);
    visit(cont->body);
    prune();
    return cont;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Parser* cont) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        LOG1("Removing " << cont);
        prune();
        return nullptr;
    }

    visit(cont->stateful);
    visit(cont->states);
    prune();
    return cont;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Table* cont) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        ::warning("Table %1% is not used; removing", cont);
        LOG1("Removing " << cont);
        cont = nullptr;
    }
    prune();
    return cont;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Declaration_Variable* decl) {
    prune();
    if (decl->initializer == nullptr)
        return process(decl);
    if (!SideEffects::check(decl->initializer, nullptr, nullptr))
        return process(decl);
    return decl;
}

const IR::Node* RemoveUnusedDeclarations::process(const IR::IDeclaration* decl) {
    LOG1("Visiting " << decl);
    auto ctx = getContext();
    if (decl->getName().name == IR::P4Program::main &&
        ctx->parent->node->is<IR::P4Program>())
        return decl->getNode();
    if (refMap->isUsed(getOriginal<IR::IDeclaration>()))
        return decl->getNode();
    LOG1("Removing " << getOriginal());
    prune();  // no need to go deeper
    return nullptr;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::ParserState* state) {
    if (state->name == IR::ParserState::accept ||
        state->name == IR::ParserState::reject ||
        state->name == IR::ParserState::start)
        return state;
    if (refMap->isUsed(getOriginal<IR::ParserState>()))
        return state;
    LOG1("Removing " << state);
    prune();
    return nullptr;
}

}  // namespace P4
