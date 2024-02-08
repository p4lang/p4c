#include "backends/p4tools/common/control_plane/symbolic_variables.h"

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools {

namespace ControlPlaneState {

const IR::SymbolicVariable *getTableActive(cstring tableName) {
    cstring label = tableName + "_configured";
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), label);
}

const IR::SymbolicVariable *getTableKey(cstring tableName, cstring keyFieldName,
                                        const IR::Type *type) {
    cstring label = tableName + "_key_" + keyFieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *getTableTernaryMask(cstring tableName, cstring keyFieldName,
                                                const IR::Type *type) {
    cstring label = tableName + "_mask_" + keyFieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *getTableMatchLpmPrefix(cstring tableName, cstring keyFieldName,
                                                   const IR::Type *type) {
    cstring label = tableName + "_lpm_prefix_" + keyFieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *getTableActionArgument(cstring tableName, cstring actionName,
                                                   cstring parameterName, const IR::Type *type) {
    cstring label = tableName + "_" + actionName + "_arg_" + parameterName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *getTableActionChoice(cstring tableName) {
    cstring label = tableName + "_action";
    return ToolsVariables::getSymbolicVariable(IR::Type_String::get(), label);
}

}  // namespace ControlPlaneState

namespace Bmv2ControlPlaneState {

const IR::SymbolicVariable *getCloneActive() {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
}

const IR::SymbolicVariable *getCloneSessionId(const IR::Type *type) {
    return ToolsVariables::getSymbolicVariable(type, "clone_session_id");
}

std::pair<const IR::SymbolicVariable *, const IR::SymbolicVariable *> getTableRange(
    cstring tableName, cstring keyFieldName, const IR::Type *type) {
    cstring minName = tableName + "_range_min_" + keyFieldName;
    cstring maxName = tableName + "_range_max_" + keyFieldName;
    return {new IR::SymbolicVariable(type, minName), new IR::SymbolicVariable(type, maxName)};
}

}  // namespace Bmv2ControlPlaneState

}  // namespace P4Tools
