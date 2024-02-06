#ifndef BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_
#define BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_

#include "ir/ir.h"
#include "ir/irutils.h"

/// Defines accessors and utility functions for state that is managed by the control plane.
/// This class can be extended by targets to customize initialization behavior and add
/// target-specific utility functions.
namespace P4Tools {

namespace ControlPlaneState {

/// @returns the symbolic boolean variable describing whether a table is configured by the
/// control plane.
const IR::SymbolicVariable *getTableActive(cstring tableName);

/// @returns the symbolic variable describing a table match key.
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableKey(cstring tableName, cstring keyFieldName,
                                        const IR::Type *type);

/// @returns the symbolic variable describing a table ternary mask.
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableTernaryMask(cstring tableName, cstring keyFieldName,
                                                const IR::Type *type);

/// @returns the symbolic variable describing a table LPM prefix match.
/// The table and field name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableMatchLpmPrefix(cstring tableName, cstring keyFieldName,
                                                   const IR::Type *type);

/// @returns the symbolic variable describing an action argument as part of a match-action call. The
/// table and action name are needed to generate a unique variable.
const IR::SymbolicVariable *getTableActionArgument(cstring tableName, cstring actionName,
                                                   cstring parameterName, const IR::Type *type);

/// @returns the symbolic variable listing the chosen action for a particular table.
const IR::SymbolicVariable *getTableActionChoice(cstring tableName);

}  // namespace ControlPlaneState

namespace Bmv2ControlPlaneState {

/// @returns the symbolic boolean variable describing whether a clone session
/// is active in the program.
const IR::SymbolicVariable *getCloneActive();

/// @returns the symbolic session id variable.
const IR::SymbolicVariable *getSessionId(const IR::Type *type);

std::pair<const IR::SymbolicVariable *, const IR::SymbolicVariable *> getTableRange(
    cstring tableName, cstring keyFieldName, const IR::Type *type);

}  // namespace Bmv2ControlPlaneState

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_SYMBOLIC_VARIABLES_H_ */
