#include "midend/removeAssertAssume.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

const IR::Node* DoRemoveAssertAssume::preorder(IR::MethodCallStatement* statement) {
    auto instance = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
    if (instance->is<P4::ExternFunction>()) {
        auto externFunc = instance->to<P4::ExternFunction>();
        if (externFunc->method->name.name == "assert") {
            LOG3("Eliminating assert call " << dbp(statement));
            return nullptr;
        } else if (externFunc->method->name.name == "assume") {
            LOG3("Eliminating assume call " << dbp(statement));
            return nullptr;
        }
    }
    return statement;
}

}  // namespace P4
