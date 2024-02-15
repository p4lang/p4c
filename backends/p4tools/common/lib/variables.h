#ifndef BACKENDS_P4TOOLS_COMMON_LIB_VARIABLES_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_VARIABLES_H_

#include "ir/ir.h"
#include "lib/cstring.h"

/// Variables internal to P4Tools. These variables do not exist in the P4
/// program itself, but are generated and added to the environment by the P4Tools tooling. These
/// variables are also used for SMT solvers as symbolic variables.
namespace P4Tools {

/// A list of constraints. These constraints may take the form of "x == 8w1","x != y", where "x" and
/// "y" are symbolic variables. They are expressed in P4C IR form and may be consumed by SMT or
/// similar solvers.
using ConstraintsVector = std::vector<const IR::Expression *>;

namespace ToolsVariables {

/// To represent header validity, we pretend that every header has a field that reflects the
/// header's validity state. This is the name of that field. This is not a valid P4 identifier,
/// so it is guaranteed to not conflict with any other field in the header.
static const cstring VALID = "*valid";

/// @returns the variable with the given @type, @incarnation, and @name.
///
/// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
/// different @type.
const IR::StateVariable &getStateVariable(const IR::Type *type, cstring name);

/// @returns the symbolic variable with the given @type, @incarnation, and @name.
///
/// A BUG occurs if this was previously called with the same @name and @incarnation, but with a
/// different @type.
const IR::SymbolicVariable *getSymbolicVariable(const IR::Type *type, cstring name);

/// @see ToolsVariables::getSymbolicVariable.
/// This function is used to generated variables caused by undefined behavior. This is merely a
/// wrapper function for the creation of a new Taint IR object.
const IR::TaintExpression *getTaintExpression(const IR::Type *type);

/// @returns the state variable for the validity of the given header instance. The resulting
///     variable will be boolean-typed.
///
/// @param headerRef a header instance. This is either a Member or a PathExpression.
IR::StateVariable getHeaderValidity(const IR::Expression *headerRef);

/// @returns an IR::Expression converted into a StateVariable. Currently only IR::PathExpression
/// and IR::Member can be converted into a state variable.
IR::StateVariable convertReference(const IR::Expression *ref);

}  // namespace ToolsVariables

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_VARIABLES_H_ */
