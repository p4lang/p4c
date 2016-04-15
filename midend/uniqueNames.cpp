#include "uniqueNames.h"

namespace P4 {

UniqueNames::UniqueNames(bool anyOrder) :
        refMap(new ReferenceMap), renameMap(new RenameMap) {
    setStopOnError(true);
    passes.emplace_back(new ResolveReferences(refMap, anyOrder));
    passes.emplace_back(new FindSymbols(refMap, renameMap));
    passes.emplace_back(new RenameSymbols(refMap, renameMap));
};

void FindSymbols::postorder(const IR::Declaration_Variable* decl) {
    cstring newName = refMap->newName(decl->getName());
    renameMap->setNewName(decl, newName);
}

void FindSymbols::postorder(const IR::Declaration_Constant* decl) {
    cstring newName = refMap->newName(decl->getName());
    renameMap->setNewName(decl, newName);
}

const IR::Node* RenameSymbols::postorder(IR::Declaration_Variable* decl) {
    // TODO
    return decl;
}

const IR::Node* RenameSymbols::postorder(IR::Declaration_Constant* decl) {
    // TODO
    return decl;
}

const IR::Node* RenameSymbols::postorder(IR::PathExpression* expression) {
    // TODO
    return expression;
}

}  // namespace P4
