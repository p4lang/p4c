#include "backends/p4tools/common/lib/zombie.h"

#include <map>
#include <string>
#include <utility>

namespace P4Tools {

const cstring Zombie::P4tZombie = "p4t*zombie";
const cstring Zombie::Const = "const";

bool Zombie::isSymbolicConst(const IR::Member* member) {
    // Should always have an inner member to represent at least p4t*zombie.const.
    const auto* innerMember = member->expr->to<IR::Member>();
    if (innerMember == nullptr) {
        return false;
    }

    // Base case: detect p4t*zombie.const.foo.
    if (innerMember->member == Const) {
        if (const auto* pathExpr = innerMember->expr->to<IR::PathExpression>()) {
            const auto* path = pathExpr->path;
            return (path != nullptr) && path->name == P4tZombie;
        }
    }

    // Recursive case.
    return isSymbolicConst(innerMember);
}

const StateVariable& Zombie::getVar(const IR::Type* type, int incarnation, cstring name) {
    return getZombie(type, false, incarnation, name);
}

const StateVariable& Zombie::getConst(const IR::Type* type, int incarnation, cstring name) {
    return getZombie(type, true, incarnation, name);
}

const StateVariable& Zombie::getZombie(const IR::Type* type, bool isConst, int incarnation,
                                       cstring name) {
    // Zombie variables are interned. Keys in the intern map are tuples of isConst, incarnations,
    // and names.
    return *mkZombie(type, isConst, incarnation, name);
}

const StateVariable* Zombie::mkZombie(const IR::Type* type, bool isConst, int incarnation,
                                      cstring name) {
    static IR::PathExpression zombieHdr(new IR::Path(P4tZombie));

    using key_t = std::pair<bool, int>;
    static std::map<key_t, const IR::Member*> incarnations;
    const auto*& incarnationMember = incarnations[std::make_pair(isConst, incarnation)];
    if (incarnationMember == nullptr) {
        const IR::Expression* hdr = &zombieHdr;
        if (isConst) {
            hdr = new IR::Member(hdr, Const);
        }
        incarnationMember = new IR::Member(hdr, cstring(std::to_string(incarnation)));
    }

    return new StateVariable(new IR::Member(type, incarnationMember, name));
}

}  // namespace P4Tools
