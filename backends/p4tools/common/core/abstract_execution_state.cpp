#include "backends/p4tools/common/core/abstract_execution_state.h"

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools {

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

AbstractExecutionState::AbstractExecutionState(const IR::P4Program *program)
    : namespaces(NamespaceContext::Empty->push(program)) {}

AbstractExecutionState::AbstractExecutionState() : namespaces(NamespaceContext::Empty) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

bool AbstractExecutionState::exists(const IR::StateVariable &var) const { return env.exists(var); }

const SymbolicEnv &AbstractExecutionState::getSymbolicEnv() const { return env; }

void AbstractExecutionState::printSymbolicEnv(std::ostream &out) const {
    // TODO(fruffy): How do we do logging here?
    out << "##### Symbolic Environment Begin #####\n";
    for (const auto &envVar : env.getInternalMap()) {
        const auto var = envVar.first;
        const auto *val = envVar.second;
        out << "Variable: " << var->toString() << " Value: " << val << '\n';
    }
    out << "##### Symbolic Environment End #####\n";
}
/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration *AbstractExecutionState::findDecl(const IR::Path *path) const {
    return namespaces->findDecl(path);
}

const IR::IDeclaration *AbstractExecutionState::findDecl(const IR::PathExpression *pathExpr) const {
    return findDecl(pathExpr->path);
}

const IR::Type *AbstractExecutionState::resolveType(const IR::Type *type) const {
    const auto *typeName = type->to<IR::Type_Name>();
    // Nothing to resolve here. Just return.
    if (typeName == nullptr) {
        return type;
    }
    const auto *path = typeName->path;
    const auto *decl = findDecl(path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

const NamespaceContext *AbstractExecutionState::getNamespaceContext() const { return namespaces; }

void AbstractExecutionState::setNamespaceContext(const NamespaceContext *namespaces) {
    this->namespaces = namespaces;
}

void AbstractExecutionState::pushNamespace(const IR::INamespace *ns) {
    namespaces = namespaces->push(ns);
}

void AbstractExecutionState::popNamespace() { namespaces = namespaces->pop(); }

/* =========================================================================================
 *  General utilities involving execution state.
 * ========================================================================================= */

const IR::Expression *AbstractExecutionState::convertToComplexExpression(
    const IR::StateVariable &parent) const {
    if (auto ts = parent->type->to<IR::Type_StructLike>()) {
        IR::IndexedVector<IR::NamedExpression> components;
        for (auto structField : ts->fields) {
            auto fieldName = structField->name;
            const auto *fieldType = resolveType(structField->type);
            auto ref = new IR::Member(fieldType, parent, fieldName);
            if (fieldType->is<IR::Type_StructLike>() || fieldType->to<IR::Type_Stack>()) {
                components.push_back(
                    new IR::NamedExpression(fieldName, convertToComplexExpression(ref)));
            } else {
                components.push_back(new IR::NamedExpression(fieldName, get(ref)));
            }
        }
        if (ts->is<IR::Type_Header>()) {
            auto validity = ToolsVariables::getHeaderValidity(parent);
            // We do not know the underlying type name of the struct, so keep it anonymous.
            return new IR::HeaderExpression(ts, nullptr, components, get(validity));
        } else {
            // We do not know the underlying type name of the struct, so keep it anonymous.
            return new IR::StructExpression(ts, nullptr, components);
        }
    } else if (auto ts = parent->type->to<IR::Type_Stack>()) {
        IR::Vector<IR::Expression> components;
        const auto *elementType = resolveType(ts->elementType);
        for (size_t idx = 0; idx < ts->getSize(); idx++) {
            auto ref = new IR::ArrayIndex(elementType, parent,
                                          new IR::Constant(static_cast<uint64_t>(idx)));
            if (elementType->is<IR::Type_StructLike>() || elementType->to<IR::Type_Stack>()) {
                components.push_back(convertToComplexExpression(ref));
            } else {
                components.push_back(get(ref));
            }
        }
        return new IR::HeaderStackExpression(parent->type, components, parent->type);
    }
    P4C_UNIMPLEMENTED("Unsupported struct-like type %1% for member %2%",
                      parent->type->node_type_name(), parent);
}

std::vector<const IR::Expression *> AbstractExecutionState::flattenComplexExpression(
    const IR::Expression *inputExpression, std::vector<const IR::Expression *> &flatValids) {
    std::vector<const IR::Expression *> exprList;
    if (const auto *structExpr = inputExpression->to<IR::StructExpression>()) {
        for (const auto *listElem : structExpr->components) {
            auto subList = flattenComplexExpression(listElem->expression, flatValids);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        }
        if (auto headerExpr = structExpr->to<IR::HeaderExpression>()) {
            flatValids.emplace_back(headerExpr->validity);
        }
    } else if (const auto *headerStackExpr = inputExpression->to<IR::HeaderStackExpression>()) {
        for (const auto *headerStackElem : headerStackExpr->components) {
            auto subList = flattenComplexExpression(headerStackElem, flatValids);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        }
    } else {
        exprList.emplace_back(inputExpression);
    }
    return exprList;
}

std::vector<IR::StateVariable> AbstractExecutionState::getFlatFields(
    const IR::StateVariable &parent, std::vector<IR::StateVariable> *validVector) const {
    std::vector<IR::StateVariable> flatFields;
    const auto *resolvedType = resolveType(parent->type);
    if (const auto *structType = resolvedType->to<IR::Type_StructLike>()) {
        for (const auto *field : structType->fields) {
            const auto *fieldType = resolveType(field->type);
            auto subFields =
                getFlatFields(new IR::Member(fieldType, parent, field->name), validVector);
            flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
        }
        // If we are dealing with a header we also include the validity bit in the list of
        // fields.
        if (validVector != nullptr && structType->is<IR::Type_Header>()) {
            validVector->push_back(ToolsVariables::getHeaderValidity(parent));
        }
    } else if (const auto *typeStack = resolvedType->to<IR::Type_Stack>()) {
        const auto *stackElementsType = resolveType(typeStack->elementType);
        for (size_t arrayIndex = 0; arrayIndex < typeStack->getSize(); arrayIndex++) {
            const auto *newArr =
                HSIndexToMember::produceStackIndex(stackElementsType, parent, arrayIndex);
            BUG_CHECK(stackElementsType->is<IR::Type_StructLike>(),
                      "Trying to get the flat fields for a non Type_StructLike element : %1%",
                      stackElementsType);
            auto subFields = getFlatFields(newArr, validVector);
            flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
        }
    } else {
        flatFields.push_back(parent);
    }
    return flatFields;
}

void AbstractExecutionState::initializeStructLike(const Target &target,
                                                  const IR::StateVariable &targetVar,
                                                  bool forceTaint) {
    std::vector<IR::StateVariable> flatTargetValids;
    auto flatTargetFields = getFlatFields(targetVar, &flatTargetValids);
    for (const auto &fieldTargetValid : flatTargetValids) {
        set(fieldTargetValid, IR::getBoolLiteral(false));
    }
    for (const auto &flatTargetRef : flatTargetFields) {
        set(flatTargetRef, target.createTargetUninitialized(flatTargetRef->type, forceTaint));
    }
}

void AbstractExecutionState::assignStructLike(const IR::StateVariable &left,
                                              const IR::Expression *right) {
    if (const auto *structExpr = right->to<IR::StructExpression>()) {
        std::vector<IR::StateVariable> flatLeftValids;
        std::vector<const IR::Expression *> flatRightValids;
        auto flatTargetFields = getFlatFields(left, &flatLeftValids);
        auto flatStructFields = flattenComplexExpression(structExpr, flatRightValids);
        BUG_CHECK(flatTargetFields.size() == flatStructFields.size(),
                  "The size of target fields (%1%) and the size of source fields (%2%) are "
                  "different.",
                  flatTargetFields.size(), flatStructFields.size());
        BUG_CHECK(
            flatLeftValids.size() == flatRightValids.size(),
            "The size of target valid fields (%1%) and the size of source valid fields (%2%) are "
            "different.",
            flatLeftValids.size(), flatRightValids.size());
        // First, complete the validity assignments for the data structure.
        for (size_t idx = 0; idx < flatLeftValids.size(); ++idx) {
            const auto &flatLeftValidRef = flatLeftValids[idx];
            const auto *flatRightValidExpr = flatRightValids[idx];
            set(flatLeftValidRef, flatRightValidExpr);
        }

        // Then, complete the assignments for the data structure.
        for (size_t idx = 0; idx < flatTargetFields.size(); ++idx) {
            const auto &flatTargetRef = flatTargetFields[idx];
            const auto *flatStructField = flatStructFields[idx];
            set(flatTargetRef, flatStructField);
        }
    } else if (auto stackExpression = right->to<IR::HeaderStackExpression>()) {
        auto stackType = stackExpression->headerStackType->checkedTo<IR::Type_Stack>();
        auto stackSize = stackExpression->components.size();
        for (size_t idx = 0; idx < stackSize; idx++) {
            const auto *ref = HSIndexToMember::produceStackIndex(stackType->elementType, left, idx);
            const auto *rightElem = stackExpression->components.at(idx);
            assignStructLike(ref, rightElem);
        }
    } else if (right->is<IR::PathExpression>() || right->is<IR::Member>() ||
               right->is<IR::ArrayIndex>()) {
        setStructLike(left, ToolsVariables::convertReference(right));
    } else {
        P4C_UNIMPLEMENTED("Unsupported assignment rval %1% of type %2%", right,
                          right->node_type_name());
    }
}

void AbstractExecutionState::setStructLike(const IR::StateVariable &targetVar,
                                           const IR::StateVariable &sourceVar) {
    std::vector<IR::StateVariable> flatTargetValids;
    std::vector<IR::StateVariable> flatSourceValids;
    auto flatTargetFields = getFlatFields(targetVar, &flatTargetValids);
    auto flatSourceFields = getFlatFields(sourceVar, &flatSourceValids);
    BUG_CHECK(flatTargetFields.size() == flatSourceFields.size(),
              "The list of target fields and the list of source fields have "
              "different sizes.");
    for (size_t idx = 0; idx < flatTargetValids.size(); ++idx) {
        const auto &fieldTargetValid = flatTargetValids[idx];
        const auto &fieldSourceTarget = flatSourceValids[idx];
        set(fieldTargetValid, get(fieldSourceTarget));
    }
    // First, complete the assignments for the data structure.
    for (size_t idx = 0; idx < flatSourceFields.size(); ++idx) {
        const auto &flatTargetRef = flatTargetFields[idx];
        const auto &fieldSourceRef = flatSourceFields[idx];
        set(flatTargetRef, get(fieldSourceRef));
    }
}

void AbstractExecutionState::declareVariable(const Target &target,
                                             const IR::Declaration_Variable &declVar) {
    if (declVar.initializer != nullptr) {
        P4C_UNIMPLEMENTED("Unsupported initializer %s for declaration variable.",
                          declVar.initializer);
    }
    const auto *declType = resolveType(declVar.type);

    // If the variable does not have an initializer we need to create a new, uninitialized value for
    // it.
    const auto *leftExpr = new IR::PathExpression(declType, new IR::Path(declVar.name.name));
    if (declType->is<IR::Type_StructLike>()) {
        initializeStructLike(target, leftExpr, false);
    } else if (const auto *stackType = declType->to<IR::Type_Stack>()) {
        const auto *stackSizeExpr = stackType->size;
        auto stackSize = stackSizeExpr->checkedTo<IR::Constant>()->asInt();
        const auto *stackElemType = stackType->elementType;
        if (stackElemType->is<IR::Type_Name>()) {
            stackElemType = resolveType(stackElemType->to<IR::Type_Name>());
        }
        const auto *structType = stackElemType->checkedTo<IR::Type_StructLike>();
        for (auto idx = 0; idx < stackSize; idx++) {
            const auto *parentExpr = HSIndexToMember::produceStackIndex(
                structType, new IR::PathExpression(stackType, new IR::Path(declVar.name.name)),
                idx);
            initializeStructLike(target, parentExpr, false);
        }
    } else if (declType->is<IR::Type_Base>()) {
        set(leftExpr, target.createTargetUninitialized(declType, false));
    } else {
        P4C_UNIMPLEMENTED("Unsupported declaration type %1% node: %2%", declType,
                          declType->node_type_name());
    }
}

void AbstractExecutionState::copyIn(const Target &target, const IR::Parameter *internalParam,
                                    cstring externalParamName) {
    const auto *paramType = resolveType(internalParam->type);
    // We can not copy externs.
    if (paramType->is<IR::Type_Extern>()) {
        return;
    }
    const auto *internalParamRef =
        new IR::PathExpression(paramType, new IR::Path(internalParam->getName().name));
    if (paramType->is<IR::Type_StructLike>()) {
        if (internalParam->direction == IR::Direction::Out) {
            initializeStructLike(target, internalParamRef, false);
        } else {
            const auto *externalParamRef =
                new IR::PathExpression(paramType, new IR::Path(externalParamName));
            setStructLike(internalParamRef, externalParamRef);
        }
    } else if (const auto *tb = paramType->to<IR::Type_Base>()) {
        if (internalParam->direction == IR::Direction::Out) {
            set(internalParamRef, target.createTargetUninitialized(tb, false));
        } else {
            const auto *externalParamRef =
                new IR::PathExpression(paramType, new IR::Path(externalParamName));
            set(internalParamRef, get(externalParamRef));
        }
    } else {
        P4C_UNIMPLEMENTED("Unsupported copy-in type %1%", paramType->node_type_name());
    }
}

void AbstractExecutionState::copyOut(const IR::Parameter *internalParam,
                                     cstring externalParamName) {
    const auto *paramType = resolveType(internalParam->type);
    // We can not copy externs.
    if (paramType->is<IR::Type_Extern>()) {
        return;
    }
    if (internalParam->direction != IR::Direction::Out &&
        internalParam->direction != IR::Direction::InOut) {
        return;
    }
    const auto *externalParamRef =
        new IR::PathExpression(paramType, new IR::Path(externalParamName));
    const auto *internalParamRef =
        new IR::PathExpression(paramType, new IR::Path(internalParam->getName().name));
    if (paramType->is<IR::Type_StructLike>()) {
        setStructLike(externalParamRef, internalParamRef);
    } else if (paramType->is<IR::Type_Base>()) {
        set(externalParamRef, get(internalParamRef));
    } else {
        P4C_UNIMPLEMENTED("Unsupported copy-in type %1%", paramType->node_type_name());
    }
}

void AbstractExecutionState::initializeBlockParams(const Target &target,
                                                   const IR::Type_Declaration *typeDecl,
                                                   const std::vector<cstring> *blockParams) {
    // Collect parameters.
    const auto *iApply = typeDecl->to<IR::IApply>();
    BUG_CHECK(iApply != nullptr, "Constructed type %s of type %s not supported.", typeDecl,
              typeDecl->node_type_name());
    // Also push the namespace of the respective parameter.
    pushNamespace(typeDecl->to<IR::INamespace>());
    // Collect parameters.
    const auto *params = iApply->getApplyParameters();
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto *param = params->getParameter(paramIdx);
        // Retrieve the identifier of the global architecture map using the parameter index.
        auto archRef = blockParams->at(paramIdx);
        // Irrelevant parameter. Ignore.
        if (archRef == nullptr) {
            continue;
        }
        // We need to resolve type names.
        const auto *paramType = resolveType(param->type);
        const auto *archPath = new IR::PathExpression(paramType, new IR::Path(archRef));
        if (paramType->is<IR::Type_StructLike>()) {
            initializeStructLike(target, archPath, false);
        } else if (paramType->is<IR::Type_Base>()) {
            set(archPath, target.createTargetUninitialized(paramType, false));
        } else {
            P4C_UNIMPLEMENTED("Unsupported initialization type %1%", paramType->node_type_name());
        }
    }
}

const IR::P4Action *AbstractExecutionState::getP4Action(
    const IR::MethodCallExpression *actionExpr) const {
    const auto *tableAction = actionExpr->checkedTo<IR::MethodCallExpression>();
    const auto *actionPath = tableAction->method->checkedTo<IR::PathExpression>();
    const auto *declaration = findDecl(actionPath);
    return declaration->checkedTo<IR::P4Action>();
}

const IR::P4Table *AbstractExecutionState::findTable(const IR::Member *member) const {
    if (member->member != IR::IApply::applyMethodName) {
        return nullptr;
    }
    if (member->expr->is<IR::PathExpression>()) {
        const auto *declaration = findDecl(member->expr->to<IR::PathExpression>());
        return declaration->to<IR::P4Table>();
    }
    const auto *type = member->expr->type;
    if (const auto *tableType = type->to<IR::Type_Table>()) {
        return tableType->table;
    }
    return nullptr;
}

}  // namespace P4Tools
