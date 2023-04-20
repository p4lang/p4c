#include "backends/p4tools/common/lib/variables.h"

#include <map>
#include <string>
#include <tuple>

#include "ir/id.h"

namespace P4Tools {

const IR::StateVariable &ToolsVariables::getStateVariable(const IR::Type *type, int incarnation,
                                                          cstring name) {
    static IR::PathExpression VAR_HDR(new IR::Path("p4t*var"));

    // StateVariables are interned. Keys in the intern map is the signedness and width of the
    // type.
    // TODO: Caching.
    return *new IR::StateVariable(
        new IR::Member(type, new IR::Member(&VAR_HDR, cstring(std::to_string(incarnation))), name));
}

const IR::SymbolicVariable *ToolsVariables::getSymbolicVariable(const IR::Type *type,
                                                                int incarnation, cstring name) {
    // static SymbolicSet SYMBOLIC_VARS;
    // TODO: Caching.
    // const auto *&incarnationMember = SYMBOLIC_VARS[incarnation];
    auto symbolicName = name + "_" + std::to_string(incarnation);
    return new IR::SymbolicVariable(type, symbolicName);
}

const cstring ToolsVariables::VALID = "*valid";

IR::StateVariable ToolsVariables::getHeaderValidity(const IR::Expression *headerRef) {
    return new IR::Member(IR::Type::Boolean::get(), headerRef, VALID);
}

IR::StateVariable ToolsVariables::addStateVariablePostfix(const IR::Expression *paramPath,
                                                          const IR::Type_Base *baseType) {
    return new IR::Member(baseType, paramPath, "*");
}

const IR::TaintExpression *ToolsVariables::getTaintExpression(const IR::Type *type) {
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
    static std::map<key_t, const IR::TaintExpression *> taints;

    auto *&result = taints[{tb->width_bits(), tb->isSigned}];
    if (result == nullptr) {
        result = new IR::TaintExpression(type);
    }

    return result;
}

}  // namespace P4Tools
