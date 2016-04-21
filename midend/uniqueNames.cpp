#include "uniqueNames.h"

namespace P4 {

// Add a @name annotation ONLY if it does not already exist.
// Otherwise do nothing.
static const IR::Annotations*
addNameAnnotation(cstring name, const IR::Annotations* annos) {
    if (annos == nullptr)
        annos = IR::Annotations::empty;
    return annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                     new IR::StringLiteral(Util::SourceInfo(), name));
}

UniqueNames::UniqueNames(bool anyOrder) :
        refMap(new ReferenceMap), renameMap(new RenameMap) {
    setStopOnError(true);
    passes.emplace_back(new ResolveReferences(refMap, anyOrder));
    passes.emplace_back(new FindSymbols(refMap, renameMap));
    passes.emplace_back(new RenameSymbols(refMap, renameMap));
}

void FindSymbols::postorder(const IR::Declaration_Variable* decl) {
    cstring newName = refMap->newName(decl->getName());
    renameMap->setNewName(decl, newName);
}

void FindSymbols::postorder(const IR::Declaration_Constant* decl) {
    cstring newName = refMap->newName(decl->getName());
    renameMap->setNewName(decl, newName);
}

/**************************************************************************/

const IR::Node* RenameSymbols::postorder(IR::Declaration_Variable* decl) {
    auto orig = getOriginal<IR::IDeclaration>();
    if (!renameMap->toRename(orig))
        return decl;
    auto newName = renameMap->getName(orig);
    auto name = IR::ID(decl->name.srcInfo, newName);
    auto annos = addNameAnnotation(decl->name, decl->annotations);
    auto result = new IR::Declaration_Variable(decl->srcInfo, name, annos,
                                               decl->type, decl->initializer);
    return result;
}

const IR::Node* RenameSymbols::postorder(IR::Declaration_Constant* decl) {
    auto orig = getOriginal<IR::IDeclaration>();
    if (!renameMap->toRename(orig))
        return decl;
    auto newName = renameMap->getName(orig);
    auto name = IR::ID(decl->name.srcInfo, newName);
    auto annos = addNameAnnotation(decl->name, decl->annotations);
    auto result = new IR::Declaration_Constant(decl->srcInfo, name, annos,
                                               decl->type, decl->initializer);
    return result;
}

const IR::Node* RenameSymbols::postorder(IR::PathExpression* expression) {
    auto decl = refMap->getDeclaration(expression->path, true);
    if (!renameMap->toRename(decl))
        return expression;
    // This should be a local name.
    BUG_CHECK(expression->path->prefix == nullptr,
              "%1%: renaming expression with path", expression);
    auto newName = renameMap->getName(decl);
    auto name = IR::ID(expression->path->name.srcInfo, newName);
    auto result = new IR::PathExpression(name);
    return result;
}

const IR::Node* RenameSymbols::preorder(IR::P4Control* control) {
    // We must visit the NameMap manually, because today mutating an
    // element inserts it at the end of the NameMap, and we care about the order.
    auto stateful = new IR::NameMap<IR::Declaration, ordered_map>();
    for (auto s : control->stateful) {
        auto decl = s.second;
        visit(decl);
        stateful->addUnique(decl->name, decl);
    }
    visit(control->body);
    auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                    control->constructorParams, std::move(*stateful),
                                    control->body);
    prune();
    return result;
}

}  // namespace P4
