#ifndef MIDEND_REMOVEASSERTASSUME_H_
#define MIDEND_REMOVEASSERTASSUME_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {
// Removes assert and assume statements if it is not in debug mode
class DoRemoveAssertAssume : public Transform, public ResolutionContext {
    P4::TypeMap *typeMap;

 public:
    explicit DoRemoveAssertAssume(P4::TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoRemoveAssertAssume");
    }

    const IR::Node *preorder(IR::MethodCallStatement *statement) override;
};

class RemoveAssertAssume final : public PassManager {
 public:
    explicit RemoveAssertAssume(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveAssertAssume(typeMap));
        setName("RemoveAssertAssume");
    }
};

}  // namespace P4

#endif /* MIDEND_REMOVEASSERTASSUME_H_ */
