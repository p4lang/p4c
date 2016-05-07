#ifndef _FRONTENDS_P4_ENUMINSTANCE_H_
#define _FRONTENDS_P4_ENUMINSTANCE_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// helps resolving references to compile-time enum fields, e.g., X.a
class EnumInstance {
    EnumInstance(const IR::Type_Enum* type, const IR::ID name) :
            type(type), name(name) {}
 public:
    const IR::Type_Enum* type;
    const IR::ID         name;

    // Returns nullptr if the expression is not a compile-time constant
    // referring to an enum
    static EnumInstance* resolve(const IR::Expression* expression, const P4::TypeMap* typeMap) {
        if (!expression->is<IR::Member>())
            return nullptr;
        auto member = expression->to<IR::Member>();
        if (!member->expr->is<IR::TypeNameExpression>())
            return nullptr;
        auto type = typeMap->getType(expression, true);
        if (!type->is<IR::Type_Enum>())
            return nullptr;
        return new EnumInstance(type->to<IR::Type_Enum>(), member->member);
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_ENUMINSTANCE_H_ */
