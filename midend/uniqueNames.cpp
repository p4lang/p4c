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
    setName("UniqueNames");
    passes.emplace_back(new ResolveReferences(refMap, anyOrder));
    passes.emplace_back(new FindSymbols(refMap, renameMap));
    passes.emplace_back(new RenameSymbols(refMap, renameMap));
}

/**************************************************************************/

IR::ID* RenameSymbols::getName() const {
    auto orig = getOriginal<IR::IDeclaration>();
    if (!renameMap->toRename(orig))
        return nullptr;
    auto newName = renameMap->getName(orig);
    auto name = new IR::ID(orig->getName().srcInfo, newName);
    return name;
}

const IR::Node* RenameSymbols::postorder(IR::Declaration_Variable* decl) {
    auto name = getName();
    if (name != nullptr)
        decl->name = *name;
    return decl;
}

const IR::Node* RenameSymbols::postorder(IR::Declaration_Constant* decl) {
    auto name = getName();
    if (name != nullptr)
        decl->name = *name;
    return decl;
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

const IR::Node* RenameSymbols::postorder(IR::Declaration_Instance* decl) {
    auto name = getName();
    if (name != nullptr && *name != decl->name) {
        auto annos = addNameAnnotation(decl->name, decl->annotations);
        decl->name = *name;
        decl->annotations = annos;
    }
    return decl;
}

const IR::Node* RenameSymbols::postorder(IR::P4Table* decl) {
    auto name = getName();
    if (name != nullptr && *name != decl->name) {
        auto annos = addNameAnnotation(decl->name, decl->annotations);
        decl->name = *name;
        decl->annotations = annos;
    }
    return decl;
}

const IR::Node* RenameSymbols::postorder(IR::P4Action* decl) {
    auto name = getName();
    if (name != nullptr && *name != decl->name) {
        auto annos = addNameAnnotation(decl->name, decl->annotations);
        decl->name = *name;
        decl->annotations = annos;
    }
    return decl;
}

}  // namespace P4
