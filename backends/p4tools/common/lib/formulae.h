#ifndef BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_

#include <functional>
#include <string>
#include <utility>

#include "ir/ir.h"
#include "ir/node.h"
#include "lib/exceptions.h"

namespace P4Tools {

/// Provides common functionality for implementing a thin wrapper around a 'const Node*' to enforce
/// invariants on which forms of IR nodes can inhabit implementations of this type. Implementations
/// must provide a static repOk(const Node*) function.
template <class Self, class Node = IR::Expression>
class AbstractRepCheckedNode : public ICastable {
 protected:
    std::reference_wrapper<const Node> node;

    // Implicit conversions to allow implementations to be treated like a Node*.
    operator const Node *() const { return &node.get(); }
    const Node &operator*() const { return node.get(); }
    const Node *operator->() const { return &node.get(); }

    /// @param classDesc a user-friendly description of the class, for reporting errors to the
    ///     user.
    explicit AbstractRepCheckedNode(const Node *node, std::string classDesc) : node(*node) {
        BUG_CHECK(Self::repOk(node), "%1%: Not a valid %2%.", node, classDesc);
    }
};

class StateVariable : public IR::Expression {
    /// The wrapped members.
    std::vector<std::pair<IR::ID, const IR::Type *>> members;

    explicit StateVariable(std::vector<std::pair<IR::ID, const IR::Type *>> members,
                           const IR::Type *type, const Util::SourceInfo &srcInfo)
        : Expression(srcInfo, type), members(std::move(members)) {}

 public:
    StateVariable(const IR::Member *member) : Expression(srcInfo, member->type) {
        const IR::Expression *expr = member;
        while (const auto *memberExpr = expr->to<IR::Member>()) {
            members.emplace_back(memberExpr->member, memberExpr->type);
            expr = memberExpr->expr;
        }
        const auto *path = expr->checkedTo<IR::PathExpression>();
        members.emplace_back(path->path->name, path->type);
    }

    StateVariable(const IR::PathExpression *path) : Expression(srcInfo, path->type) {
        type = path->type;
        srcInfo = path->getSourceInfo();
        members.emplace_back(path->path->name, path->type);
    }

    /// Implements comparisons so that StateVariables can be used as map keys.
    bool operator==(const StateVariable &other) const {
        // We use a custom compare function.
        auto memberSize = members.size();
        auto otherMemberSize = members.size();
        if (memberSize != otherMemberSize) {
            return false;
        }
        for (size_t idx = 0; idx < memberSize; ++idx) {
            auto member = members.at(idx);
            auto otherMember = other.members.at(idx);
            if (member.first.name != otherMember.first.name ||
                member.second != otherMember.second) {
                return false;
            }
        }

        return true;
    }

    /// Implements comparisons so that StateVariables can be used as map keys.
    [[nodiscard]] bool equiv(const StateVariable &other) const {
        // We use a custom compare function.
        auto memberSize = members.size();
        auto otherMemberSize = members.size();
        if (memberSize != otherMemberSize) {
            return false;
        }
        for (size_t idx = 0; idx < memberSize; ++idx) {
            auto member = members.at(idx);
            auto otherMember = other.members.at(idx);
            if (member.first.name != otherMember.first.name ||
                member.second->equiv(*otherMember.second)) {
                return false;
            }
        }
        return true;
    }

    /// Implements comparisons so that StateVariables can be used as map keys.
    bool operator<(const StateVariable &other) const {
        // We use a custom compare function.
        auto memberSize = members.size();
        auto otherMemberSize = members.size();
        if (memberSize < otherMemberSize) {
            return true;
        }
        for (size_t idx = 0; idx < memberSize; ++idx) {
            auto member = members.at(idx);
            auto otherMember = other.members.at(idx);
            if (member.first.name < otherMember.first.name) {
                return true;
            }
            if (member.first.name > otherMember.first.name) {
                return false;
            }
        }

        return false;
    }

    [[nodiscard]] cstring toString() const override {
        std::stringstream stream;
        for (const auto &member : members) {
            stream << member.first;
            if (&member != &members.front()) {
                stream << ".";
            }
        }
        return stream.str();
    }

    void dbprint(std::ostream &out) const override {
        for (const auto &member : members) {
            out << member.first;
            if (&member != &members.front()) {
                out << ".";
            }
        }
    }

    [[nodiscard]] const auto &getMembers() const { return members; }

    [[nodiscard]] auto getHead() const { return members.back(); }

    [[nodiscard]] auto at(size_t n) const { return members.at(n); }

    [[nodiscard]] auto getTail() const { return members.front(); }

    [[nodiscard]] auto size() const { return members.size(); }

    [[nodiscard]] StateVariable *clone() const override {
        return new StateVariable(members, type, srcInfo);
    }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_ */
