#ifndef FRONTENDS_P4_REMOVEASSERTASSUME_H_
#define FRONTENDS_P4_REMOVEASSERTASSUME_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "typeMap.h"
#include "ir/ir.h"

namespace P4 {
// Removes assert and assume statements if it is not in debug mode
class RemoveAssertAssume : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
 public:
    RemoveAssertAssume(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);

    const IR::Node* preorder(IR::MethodCallStatement* statement);
};

}  // namespace P4

#endif  /* FRONTENDS_P4_REMOVEASSERTASSUME_H_ */
