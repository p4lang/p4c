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
 public:
    /// The member of a StateVariable. Requires a type and a name.
    class VariableMember {
     public:
        /// The type of the variable item.
        const IR::Type *type;

        /// The name of the variable item.
        cstring name;

        VariableMember(const VariableMember &) = default;
        VariableMember(VariableMember &&) = default;
        VariableMember &operator=(const VariableMember &) = default;
        VariableMember &operator=(VariableMember &&) = default;
        ~VariableMember() = default;

        /// Construct with type and cstring.
        VariableMember(const IR::Type *type, cstring name);  // NOLINT(runtime/explicit)
    };

 private:
    /// The wrapped members.
    std::deque<VariableMember> members;

    /// A constructor for an anonymous, untyped state variable without type information.
    StateVariable(std::initializer_list<cstring> untypedMembers);

 public:
    StateVariable(const StateVariable &) = default;
    StateVariable(StateVariable &&) = default;
    StateVariable &operator=(const StateVariable &) = default;
    StateVariable &operator=(StateVariable &&) = default;
    ~StateVariable() override = default;

    /// Derives a StateVariable from a member by unraveling the member.
    /// The type of the StateVariable is the type of the member, and with that the last element.
    explicit StateVariable(const IR::Member &member);

    /// Derives a StateVariable from a PathExpression, which is just the path contained within the
    /// expression. The type of the StateVariable is the type of the path.
    explicit StateVariable(const IR::PathExpression &path);

    /// Derives a StateVariable from a deque of members.
    /// If the list is not empty the type of the StateVariable is the last element in the deque.
    explicit StateVariable(const Util::SourceInfo &srcInfo, std::deque<VariableMember> members);

    /// Derives a StateVariable from a deque of members.
    /// If the list is not empty the type of the StateVariable is the last element in the deque.
    explicit StateVariable(std::deque<VariableMember> members);

    /// A completely untyped StateVariable.
    /// Useful if you need to initialize an anonymous variable which is typed later.
    // NOLINTNEXTLINE
    [[nodiscard]] static StateVariable UntypedStateVariable(
        std::initializer_list<cstring> untypedMembers);

    /// Implements comparisons so that StateVariables can be used as map keys.
    bool operator==(const StateVariable &other) const;

    /// Implements comparisons so that StateVariables can be used as map keys.
    [[nodiscard]] bool equiv(const StateVariable &other) const;

    /// Implements comparisons so that StateVariables can be used as map keys.
    bool operator<(const StateVariable &other) const;

    [[nodiscard]] cstring toString() const override;

    void dbprint(std::ostream &out) const override;

    /// @returns the list of members contained in this StateVariable.
    [[nodiscard]] const std::deque<VariableMember> &getMembers() const;

    /// @returns the element at index @param n.
    [[nodiscard]] const StateVariable::VariableMember &at(size_t n) const;

    /// @returns the size of the state variable in terms of members.
    [[nodiscard]] size_t size() const;

    /// @returns the first member (from the right).
    [[nodiscard]] const StateVariable::VariableMember &front() const;

    /// @returns the last member (from the right).
    [[nodiscard]] const StateVariable::VariableMember &back() const;

    void popFrontInPlace();

    /// Pops the first member (from the right) from the StateVariable and then @returns a new
    /// variable.
    [[nodiscard]] StateVariable popFront() const;

    /// Append a member to the StateVariable in place. Updates the type of the member.
    void popBackInPlace();

    /// Pops the last member (from the right) from the StateVariable and then @returns a new
    /// variable.
    [[nodiscard]] StateVariable popBack() const;

    /// Append a member to the StateVariable in place. Updates the type of the member.
    void appendInPlace(VariableMember item);

    /// Append a member to the StateVariable. @returns a new variable.
    [[nodiscard]] StateVariable append(VariableMember &item) const;

    /// Prepend a member to the StateVariable in place.
    void prependInPlace(VariableMember item);

    /// Prepend a member to the StateVariable. @returns a new variable.
    [[nodiscard]] StateVariable prepend(VariableMember item) const;

    /// @returns a StateVariable which contains a subset of the parent variable.
    /// TODO: Use a std::span once we have C++20.
    [[nodiscard]] StateVariable getSlice(int64_t lo, int64_t hi) const;

    [[nodiscard]] StateVariable *clone() const override;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_ */
