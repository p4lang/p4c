#ifndef BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_

#include <functional>
#include <string>

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

    DECLARE_TYPEINFO(AbstractRepCheckedNode);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_FORMULAE_H_ */
