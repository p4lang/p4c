#ifndef MIDEND_ELIMINATETYPEDEFS_H_
#define MIDEND_ELIMINATETYPEDEFS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Replaces typedef by the type it was defined to represent.
 * This can be done when all the information required by the
 * control-plane has been generated.
 */
class DoReplaceTypedef final : public Transform, public ResolutionContext {
 public:
    DoReplaceTypedef() = default;
    const IR::Type *preorder(IR::Type_Name *type) override;
};

class EliminateTypedef final : public PassManager {
 public:
    EliminateTypedef(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceTypedef);
        passes.push_back(new TypeChecking(nullptr, typeMap, true));
        passes.push_back(new ClearTypeMap(typeMap)), setName("EliminateTypedefs");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATETYPEDEFS_H_ */
