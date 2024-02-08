#ifndef BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_
#define BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_

#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4Tools {

/// Defines accessors and utility functions for state that is managed by the control plane.
/// This class can be extended by targets to customize initialization behavior and add
/// target-specific utility functions.
namespace ControlPlaneState {

/// @returns the symbolic boolean variable describing whether a table is configured by the
/// control plane.
const IR::SymbolicVariable *getTableActive(cstring tableName);

/// @returns the symbolic variable describing a table match key.
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableKey(cstring tableName, cstring keyFieldName,
                                        const IR::Type *type);

/// @returns the symbolic variable representing the mask for a table ternary match.
/// The symbolic mask may be applied to the left and right-hand side of a key match
/// (|x| & |mask|== |y| & |mask|).
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableTernaryMask(cstring tableName, cstring keyFieldName,
                                                const IR::Type *type);

/// @returns the symbolic variable describing a table LPM prefix match.
/// The symbolic prefix may be applied to the left and right-hand side of a key match
/// (|x| << |lpm_prefix| == |y| << |lpm_prefix|).
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableMatchLpmPrefix(cstring tableName, cstring keyFieldName,
                                                   const IR::Type *type);

/// @returns the symbolic variable describing an action argument as part of a match-action call. The
/// table, action and argument name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableActionArgument(cstring tableName, cstring actionName,
                                                   cstring parameterName, const IR::Type *type);

/// @returns the symbolic variable listing the chosen action for a particular table.
const IR::SymbolicVariable *getTableActionChoice(cstring tableName);

}  // namespace ControlPlaneState

namespace Bmv2ControlPlaneState {

/// @returns the symbolic boolean variable describing whether a clone session
/// is active in the program.
const IR::SymbolicVariable *getCloneActive();

/// @returns the symbolic clone session id variable.
/// See also https://p4.org/p4-spec/p4runtime/main/P4Runtime-Spec.html#sec-clonesessionentry
const IR::SymbolicVariable *getCloneSessionId(const IR::Type *type);

/// @returns the symbolic variable describing a table Range match.
/// The table and field name are needed to generate a unique variable.
/// See also https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md#range-tables
std::pair<const IR::SymbolicVariable *, const IR::SymbolicVariable *> getTableRange(
    cstring tableName, cstring keyFieldName, const IR::Type *type);

}  // namespace Bmv2ControlPlaneState

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_ */
