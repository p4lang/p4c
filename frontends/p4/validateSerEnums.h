#ifndef FRONTENDS_P4_VALIDATESERENUMS_H_
#define FRONTENDS_P4_VALIDATESERENUMS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class ValidateSerEnums final : public Inspector {
 public:
    explicit ValidateSerEnums(P4::ReferenceMap *refMap) : refMap(refMap) {}

 private:
    void postorder(const IR::SerEnumMember *) override;

    P4::ReferenceMap *refMap;
};

}  // namespace P4
#endif /* FRONTENDS_P4_VALIDATESERENUMS_H_ */
