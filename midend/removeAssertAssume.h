#ifndef _MIDEND_REMOVEASSERTASSUME_H_
#define _MIDEND_REMOVEASSERTASSUME_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {
// Removes assert and assume statements if it is not in debug mode
class DoRemoveAssertAssume : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
 public:
    explicit DoRemoveAssertAssume(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
             : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("DoRemoveAssertAssume");
    }

    const IR::Node* preorder(IR::MethodCallStatement* statement);
};

class RemoveAssertAssume final : public PassManager {
 public:
    RemoveAssertAssume(ReferenceMap* refMap, TypeMap* typeMap,
                     TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveAssertAssume(refMap, typeMap));
        setName("RemoveAssertAssume");
    }
};

}  // namespace P4

#endif  /* _MIDEND_REMOVEASSERTASSUME_H_*/
