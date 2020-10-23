#ifndef EXTENSIONS_P4C_MIDEND_ELIM_TYPEDEF_H_
#define EXTENSIONS_P4C_MIDEND_ELIM_TYPEDEF_H_

#include "frontends/p4/typeChecking/typeChecker.h"

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
  EliminateTypedef(ReferenceMap *refMap, TypeMap *typeMap,
                   TypeChecking *typeChecking = nullptr) {
    if (!typeChecking)
      typeChecking = new TypeChecking(refMap, typeMap);
    passes.push_back(typeChecking);
    passes.push_back(new DoReplaceTypedef(refMap));
    passes.push_back(new TypeChecking(refMap, typeMap, true));
    setName("EliminateTypedef");
  }
};

} // namespace P4

#endif /* EXTENSIONS_BF_P4C_MIDEND_ELIM_TYPEDEF_H_ */