#include "backends/p4tools/common/lib/zombie.h"

#include <map>
#include <string>
#include <utility>

#include "ir/id.h"

namespace P4Tools {

const IR::StateVariable &Zombie::getStateVariable(const IR::Type *type, int incarnation,
                                                  cstring name) {
    static IR::PathExpression VAR_HDR(new IR::Path("p4t*var"));

    // StateVariables are interned. Keys in the intern map is the signedness and width of the
    // type.
    // TODO: Caching.
    return *new IR::StateVariable(
        new IR::Member(type, new IR::Member(&VAR_HDR, cstring(std::to_string(incarnation))), name));
}

const IR::SymbolicVariable *Zombie::getSymbolicVariable(const IR::Type *type, int incarnation,
                                                        cstring name) {
    // static SymbolicSet SYMBOLIC_VARS;
    // TODO: Caching.
    // const auto *&incarnationMember = SYMBOLIC_VARS[incarnation];
    auto symbolicName = name + "_" + std::to_string(incarnation);
    return new IR::SymbolicVariable(type, symbolicName);
}

}  // namespace P4Tools
