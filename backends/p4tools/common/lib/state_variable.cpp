#include "backends/p4tools/common/lib/formulae.h"

namespace P4Tools {

StateVariable::VariableMember::VariableMember(const IR::Type *type, cstring name)
    : type(type), name(name) {}

StateVariable::StateVariable(std::initializer_list<cstring> untypedMembers)
    : Expression(Util::SourceInfo(), IR::Type_Unknown::get()) {
    for (auto untypedMember : untypedMembers) {
        members.emplace_back(type, untypedMember);
    }
}

StateVariable::StateVariable(const IR::Member &member) : Expression(srcInfo, member.type) {
    const IR::Expression *expr = &member;
    while (const auto *memberExpr = expr->to<IR::Member>()) {
        members.emplace_front(memberExpr->type, memberExpr->member.name);
        expr = memberExpr->expr;
    }
    const auto *path = expr->checkedTo<IR::PathExpression>();
    members.emplace_front(path->type, path->path->name.name);
}

StateVariable::StateVariable(const IR::PathExpression &path) : Expression(srcInfo, path.type) {
    type = path.type;
    srcInfo = path.getSourceInfo();
    members.emplace_front(path.type, path.path->name.name);
}

StateVariable::StateVariable(const Util::SourceInfo &srcInfo, std::deque<VariableMember> members)
    : Expression(srcInfo, members.empty() ? IR::Type_Unknown::get() : members.back().type),
      members(std::move(members)) {}

StateVariable::StateVariable(std::deque<VariableMember> members)
    : Expression(members.empty() ? IR::Type_Unknown::get() : members.back().type),
      members(std::move(members)) {}

StateVariable StateVariable::UntypedStateVariable(std::initializer_list<cstring> untypedMembers) {
    return {untypedMembers};
}

bool StateVariable::operator==(const StateVariable &other) const {
    // We use a custom compare function.
    auto memberSize = members.size();
    auto otherMemberSize = members.size();
    if (memberSize != otherMemberSize) {
        return false;
    }
    for (size_t idx = 0; idx < memberSize; ++idx) {
        auto member = members.at(idx);
        auto otherMember = other.members.at(idx);
        if (member.name != otherMember.name || member.type != otherMember.type) {
            return false;
        }
    }
    return true;
}

bool StateVariable::equiv(const StateVariable &other) const {
    // We use a custom compare function.
    auto memberSize = members.size();
    auto otherMemberSize = members.size();
    if (memberSize != otherMemberSize) {
        return false;
    }
    for (size_t idx = 0; idx < memberSize; ++idx) {
        auto member = members.at(idx);
        auto otherMember = other.members.at(idx);
        if (member.name != otherMember.name || member.type->equiv(*otherMember.type)) {
            return false;
        }
    }
    return true;
}

bool StateVariable::operator<(const StateVariable &other) const {
    // We use a custom compare function.
    auto memberSize = members.size();
    auto otherMemberSize = members.size();
    if (memberSize < otherMemberSize) {
        return true;
    }
    for (size_t idx = 0; idx < memberSize; ++idx) {
        auto member = members.at(idx);
        auto otherMember = other.members.at(idx);
        if (member.name < otherMember.name) {
            return true;
        }
        if (member.name > otherMember.name) {
            return false;
        }
    }

    return false;
}

cstring StateVariable::toString() const {
    std::stringstream stream;
    for (const auto &member : members) {
        stream << member.name;
        if (&member != &members.front()) {
            stream << ".";
        }
    }
    return stream.str();
}

void StateVariable::dbprint(std::ostream &out) const {
    for (const auto &member : members) {
        out << member.name;
        if (&member != &members.front()) {
            out << ".";
        }
    }
}

const std::deque<StateVariable::VariableMember> &StateVariable::getMembers() const {
    return members;
}

const StateVariable::VariableMember &StateVariable::at(size_t n) const { return members.at(n); }

size_t StateVariable::size() const { return members.size(); }

const StateVariable::VariableMember &StateVariable::front() const { return members.front(); }

const StateVariable::VariableMember &StateVariable::back() const { return members.back(); }

void StateVariable::popFrontInPlace() {
    members.pop_front();
    type = members.back().type;
}

StateVariable StateVariable::popFront() const {
    auto var = StateVariable(srcInfo, members);
    var.popFrontInPlace();
    return var;
}

void StateVariable::popBackInPlace() {
    members.pop_back();
    type = members.back().type;
}

StateVariable StateVariable::popBack() const {
    auto var = StateVariable(srcInfo, members);
    var.popBackInPlace();
    return var;
}

void StateVariable::appendInPlace(VariableMember item) {
    type = item.type;
    members.emplace_back(item);
}

StateVariable StateVariable::append(VariableMember &item) const {
    auto var = StateVariable(srcInfo, members);
    var.appendInPlace(item);
    return var;
}

void StateVariable::prependInPlace(VariableMember item) { members.emplace_front(item); }

StateVariable StateVariable::prepend(VariableMember item) const {
    auto var = StateVariable(srcInfo, members);
    var.prependInPlace(item);
    return var;
}

StateVariable StateVariable::getSlice(int64_t lo, int64_t hi) const {
    auto numMembers = static_cast<int64_t>(members.size());
    if (lo >= hi) {
        BUG("Slice low %1% is higher than or equal to slice high %2%.", lo, hi);
    }
    if (hi >= numMembers || lo < 0) {
        BUG("Slice low %1% or slice high %2% are out of bounds (Member size %3%).", lo, hi,
            numMembers);
    }
    auto slice = std::deque<VariableMember>(members.begin() + lo, members.begin() + hi);
    return StateVariable(srcInfo, slice);
}

StateVariable *StateVariable::clone() const { return new StateVariable(srcInfo, members); }

}  // namespace P4Tools
