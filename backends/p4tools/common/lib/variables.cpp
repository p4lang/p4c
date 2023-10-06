#include "backends/p4tools/common/lib/variables.h"

#include <map>
#include <string>
#include <tuple>

#include "ir/id.h"

namespace P4Tools::ToolsVariables {

/// The prefix used for state variables.
static const IR::PathExpression VAR_PREFIX = IR::PathExpression("p4tools*var");

const IR::StateVariable &getStateVariable(const IR::Type *type, cstring name) {
    // TODO: Is caching worth it here?
    // TODO: Do we really need to "allocate" a state variable? They are immediately consumed  by the
    // symbolic environment.
    return *new IR::StateVariable(new IR::Member(type, &VAR_PREFIX, name));
}

const IR::SymbolicVariable *getSymbolicVariable(const IR::Type *type, cstring name) {
    // TODO: Is caching worth it here?
    return new IR::SymbolicVariable(type, name);
}

IR::StateVariable getHeaderValidity(const IR::Expression *headerRef) {
    return new IR::Member(IR::Type::Boolean::get(), headerRef, VALID);
}

const IR::TaintExpression *getTaintExpression(const IR::Type *type) {
    // Do not cache varbits.
    if (type->is<IR::Extracted_Varbits>()) {
        return new IR::TaintExpression(type);
    }
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto *tb = type->to<IR::Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new IR::TaintExpression(type);
    }
    // Taint expressions are interned. Keys in the intern map is the signedness and width of the
    // type.
    using key_t = std::tuple<int, bool>;
    static std::map<key_t, const IR::TaintExpression *> TAINTS;

    auto *&result = TAINTS[{tb->width_bits(), tb->isSigned}];
    if (result == nullptr) {
        result = new IR::TaintExpression(type);
    }

    return result;
}

IR::StateVariable convertReference(const IR::Expression *ref) {
    if (const auto *member = ref->to<IR::Member>()) {
        return member;
    }
    if (const auto *arrIndex = ref->to<IR::ArrayIndex>()) {
        return arrIndex;
    }
    // Local variable.
    const auto *path = ref->checkedTo<IR::PathExpression>();
    return path;
}

}  // namespace P4Tools::ToolsVariables
