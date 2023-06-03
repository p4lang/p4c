#ifndef BACKENDS_P4TOOLS_COMMON_LIB_SYMBOLIC_ENV_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_SYMBOLIC_ENV_H_

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"
#include "ir/node.h"

namespace P4Tools {

/// A symbolic environment maps variables to their symbolic value. A symbolic value is just an
/// expression on the program's initial state.
class SymbolicEnv {
 private:
    SymbolicMapType map;

 public:
    // Maybe coerce from Model for concrete execution?

    /// @returns the symbolic value for the given variable.
    [[nodiscard]] const IR::Expression *get(const IR::StateVariable &var) const;

    /// Checks whether the given variable exists in the symbolic environment.
    [[nodiscard]] bool exists(const IR::StateVariable &var) const;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding is
    /// done on the given value before updating the symbolic state.
    void set(const IR::StateVariable &var, const IR::Expression *value);

    /// Substitutes state variables in @expr for their symbolic value in this environment.
    /// Variables that are unbound by this environment are left untouched.
    const IR::Expression *subst(const IR::Expression *expr) const;

    /// @returns The immutable map that is internal to this symbolic environment.
    [[nodiscard]] const SymbolicMapType &getInternalMap() const;

    /// Determines whether the given node represents a symbolic value. Symbolic values may be
    /// stored in the symbolic environment.
    static bool isSymbolicValue(const IR::Node *);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_SYMBOLIC_ENV_H_ */
