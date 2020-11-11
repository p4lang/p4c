#include "elimTypedef.h"

namespace P4 {

const IR::Type *DoReplaceTypedef::preorder(IR::Type_Name *type) {
    if (auto decl = refMap->getDeclaration(type->path)) {
        if (!decl->getNode()->is<IR::Type_Typedef>())
            return type;
        auto tdecl = decl->getNode()->to<IR::Type_Typedef>();
        return tdecl->type;
    }
    return type;
}

} // namespace P4
