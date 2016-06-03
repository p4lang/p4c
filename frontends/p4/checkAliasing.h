#ifndef _FRONTENDS_P4_CHECKALIASING_H_
#define _FRONTENDS_P4_CHECKALIASING_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class CheckAliasing : public Inspector {
    const ReferenceMap* refMap;
    const TypeMap*      typeMap;
 public:
    CheckAliasing(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("CheckAliasing"); }
    bool preorder(const IR::MethodCallExpression* expression) override;
};

}  // namespace P4


#endif /* _FRONTENDS_P4_CHECKALIASING_H_ */
