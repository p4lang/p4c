#ifndef MIDEND_ELIMINATETYPEDEFS_H_
#define MIDEND_ELIMINATETYPEDEFS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/safe_vector.h"

namespace P4 {

/**
 * Replaces typedef by the type it was defined to represent.
 * This can be done when all the information required by the
 * control-plane has been generated.
 */
class DoReplaceTypedef final : public Transform {
    const ReferenceMap *refMap;

 public:
    explicit DoReplaceTypedef(const ReferenceMap *refMap) : refMap(refMap) {}
    const IR::Type *preorder(IR::Type_Name *type) override;
};

class EliminateTypedef final : public PassManager {
 public:
    EliminateTypedef(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceTypedef(refMap));
        passes.push_back(new TypeChecking(refMap, typeMap, true));
        setName("EliminateTypedefs");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATETYPEDEFS_H_ */
