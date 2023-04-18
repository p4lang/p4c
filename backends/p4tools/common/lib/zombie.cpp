#include "backends/p4tools/common/lib/zombie.h"

#include <map>
#include <string>
#include <utility>

#include "ir/id.h"

namespace P4Tools {

const cstring Zombie::P4tZombie = "p4t*zombie";
const cstring Zombie::Const = "const";

bool Zombie::isSymbolicConst(const IR::StateVariable &var) {
    // Should always have an inner member to represent at least p4t*zombie.const.
    if (var.size() < 2) {
        return false;
    }
    return var.front().name == P4tZombie;
}

const IR::StateVariable *Zombie::getVar(const IR::Type *type, int incarnation, cstring name) {
    return getZombie(type, false, incarnation, name);
}

const IR::StateVariable *Zombie::getConst(const IR::Type *type, int incarnation, cstring name) {
    return getZombie(type, true, incarnation, name);
}

const IR::StateVariable *Zombie::getZombie(const IR::Type *type, bool isConst, int incarnation,
                                           cstring name) {
    // Zombie variables are interned. Keys in the intern map are tuples of isConst, incarnations,
    // and names.
    return mkZombie(type, isConst, incarnation, name);
}

const IR::StateVariable *Zombie::mkZombie(const IR::Type *type, bool isConst, int incarnation,
                                          cstring name) {
    using key_t = std::pair<bool, int>;
    static std::map<key_t, const IR::StateVariable *> INCARNATIONS;
    const auto *&incarnationMember = INCARNATIONS[std::make_pair(isConst, incarnation)];
    std::deque<IR::StateVariable::VariableMember> members;
    if (incarnationMember == nullptr) {
        members.emplace_back(IR::Type_Unknown::get(), P4tZombie);
        if (isConst) {
            members.emplace_back(IR::Type_Unknown::get(), Const);
        }
        members.emplace_back(IR::Type_Unknown::get(), std::to_string(incarnation));
    }
    members.emplace_back(type, name);
    return new IR::StateVariable(members);
}

}  // namespace P4Tools
