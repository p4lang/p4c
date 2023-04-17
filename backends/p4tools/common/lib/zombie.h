#ifndef BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_

#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools {

/// Zombies are variables internal to P4Tools. They are variables that do not exist in the P4
/// program itself, but are generated and added to the environment by the P4Tools tooling. These
/// variables are also used for SMT solvers as symbolic variables.
class Zombie {
 private:
    /// The name of the top-level struct containing all zombie state.
    static const cstring P4tZombie;

    /// The name of the struct below P4tZombie that contains all symbolic constants.
    static const cstring Const;

 public:
    /// Determines whether the given member expression represents a symbolic constant. Symbolic
    /// constants are references to fields under the nested struct p4t*zombie.const.
    static bool isSymbolicConst(const IR::Member *);

    /// @returns the zombie variable with the given @type, @incarnation, and @name.
    ///
    /// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
    /// different @type.
    static const IR::StateVariable &getVar(const IR::Type *type, int incarnation, cstring name);

    /// @returns the zombie symbolic constant with the given @type, @incarnation, and @name.
    ///
    /// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
    /// different @type.
    static const IR::StateVariable &getConst(const IR::Type *type, int incarnation, cstring name);

 private:
    /// @see getVar and getConst.
    static const IR::StateVariable &getZombie(const IR::Type *type, bool isConst, int incarnation,
                                              cstring name);

    static const IR::StateVariable *mkZombie(const IR::Type *type, bool isConst, int incarnation,
                                             cstring name);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_ZOMBIE_H_ */
