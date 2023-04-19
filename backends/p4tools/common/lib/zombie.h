#ifndef BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_

#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools {

/// Zombies are variables internal to P4Tools. They are variables that do not exist in the P4
/// program itself, but are generated and added to the environment by the P4Tools tooling. These
/// variables are also used for SMT solvers as symbolic variables.
class Zombie {
 public:
    /// @returns the zombie variable with the given @type, @incarnation, and @name.
    ///
    /// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
    /// different @type.
    static const IR::StateVariable &getStateVariable(const IR::Type *type, int incarnation,
                                                     cstring name);

    /// @returns the zombie symbolic constant with the given @type, @incarnation, and @name.
    ///
    /// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
    /// different @type.
    static const IR::SymbolicVariable *getSymbolicVariable(const IR::Type *type, int incarnation,
                                                           cstring name);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_ */
