#ifndef _FRONTENDS_P4_CHECKALIASING_H_
#define _FRONTENDS_P4_CHECKALIASING_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

class DoCheckAliasing : public Inspector {
    const ReferenceMap* refMap;
    const TypeMap*      typeMap;
 public:
    DoCheckAliasing(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoCheckAliasing"); }
    bool preorder(const IR::MethodCallExpression* expression) override;
};

class CheckAliasing : public PassManager {
 public:
    CheckAliasing(ReferenceMap* refMap, TypeMap* typeMap, bool isv1) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap, isv1));
        passes.push_back(new DoCheckAliasing(refMap, typeMap));
        setName("CheckAliasing");
    }
};

}  // namespace P4


#endif /* _FRONTENDS_P4_CHECKALIASING_H_ */
