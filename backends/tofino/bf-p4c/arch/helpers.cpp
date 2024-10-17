/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/arch/helpers.h"

namespace BFN {

/**
 * Auxiliary function to get a declaration instance based on a path expression from a reference map.
 * @param[in] refMap Reference map
 * @param[in] path The path expression
 * @return Returns the declaration if exists, nullptr otherwise.
 */
const IR::Declaration_Instance *
getDeclInst(const P4::ReferenceMap *refMap, const IR::PathExpression *path) {
    auto *decl = refMap->getDeclaration(path->path);
    if (!decl) return nullptr;
    auto *decl_inst = decl->to<IR::Declaration_Instance>();
    return decl_inst;
}

std::optional<cstring> getExternTypeName(const P4::ExternMethod* extMethod) {
    std::optional<cstring> name = std::nullopt;
    if (auto inst = extMethod->object->to<IR::Declaration_Instance>()) {
        if (auto tn = inst->type->to<IR::Type_Name>()) {
            name = tn->path->name;
        } else if (auto ts = inst->type->to<IR::Type_Specialized>()) {
            if (auto bt = ts->baseType->to<IR::Type_Name>()) {
                name = bt->path->name; } }
    } else if (auto param = extMethod->object->to<IR::Parameter>()) {
        if (auto tn = param->type->to<IR::Type_Name>()) {
            name = tn->path->name;
        } else if (auto te = param->type->to<IR::Type_Extern>()) {
            name = te->name;
        }
    }
    return name;
}

/**
 * Helper functions to handle list of extern instances from table properties.
 */
std::optional<P4::ExternInstance>
getExternInstanceFromPropertyByTypeName(const IR::P4Table* table,
                                        cstring propertyName,
                                        cstring externTypeName,
                                        P4::ReferenceMap* refMap,
                                        P4::TypeMap* typeMap,
                                        bool *isConstructedInPlace) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return std::nullopt;
    }
    auto expr = property->value->to<IR::ExpressionValue>()->expression;

    std::vector<P4::ExternInstance> rv;
    auto process_extern_instance = [&] (const IR::Expression* expr) {
        if (isConstructedInPlace) *isConstructedInPlace = expr->is<IR::ConstructorCallExpression>();
        if (expr->is<IR::ConstructorCallExpression>()
                && property->getAnnotation(IR::Annotation::nameAnnotation) == nullptr) {
            error(ErrorType::ERR_UNSUPPORTED,
                    "Table '%1%' has an anonymous table property '%2%' with no name annotation, "
                    "which is not supported by P4Runtime", table->controlPlaneName(), propertyName);
        }
        auto name = property->controlPlaneName();
        auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap, name);
        if (!externInstance) {
            error(ErrorType::ERR_INVALID,
                    "Expected %1% property value for table %2% to resolve to an "
                    "extern instance: %3%", propertyName, table->controlPlaneName(),
                    property);
        }
        if (externInstance->type->name == externTypeName) {
            rv.push_back(*externInstance);
        }
    };
    if (expr->is<IR::ListExpression>()) {
        for (auto inst : expr->to<IR::ListExpression>()->components) {
            process_extern_instance(inst); }
    } else if (expr->is<IR::StructExpression>()) {
        for (auto inst : expr->to<IR::StructExpression>()->components) {
            process_extern_instance(inst->expression); }
    } else {
        process_extern_instance(expr); }

    if (rv.empty())
        return std::nullopt;

    if (rv.size() > 1) {
        error(ErrorType::ERR_UNSUPPORTED,
                "Table '%1%' has more than one extern with type '%2%' attached to "
                "property '%3%', which is not supported by Tofino", table->controlPlaneName(),
                externTypeName, propertyName);
    }
    return rv[0];
}

/**
 * Helper functions to extract extern instance from table properties.
 * Originally implemented in as part of the control-plane repo.
 */
std::optional<P4::ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              cstring propertyName,
                              P4::ReferenceMap* refMap,
                              P4::TypeMap* typeMap) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        error("Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return std::nullopt;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    auto name = property->controlPlaneName();
    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        error("Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%", propertyName, table->controlPlaneName(),
                property);
        return std::nullopt; }

    return externInstance;
}

std::optional<const IR::ExpressionValue*>
getExpressionFromProperty(const IR::P4Table* table,
                          const cstring& propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        error("Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return std::nullopt;
    }

    auto expr = property->value->to<IR::ExpressionValue>();
    return expr;
}

void convertConcatToList(std::vector<const IR::Expression*>& slices,
        const IR::Concat* expr) {
    if (expr->left->is<IR::Constant>()) {
        slices.push_back(expr->left);
    } else if (auto lhs = expr->left->to<IR::Concat>()) {
        convertConcatToList(slices, lhs);
    } else {
        slices.push_back(expr->left); }

    if (expr->right->is<IR::Constant>()) {
        slices.push_back(expr->right);
    } else if (auto rhs = expr->right->to<IR::Concat>()) {
        convertConcatToList(slices, rhs);
    } else {
        slices.push_back(expr->right);
    }
}

std::vector<const IR::Expression*>
convertConcatToList(const IR::Concat* expr) {
    std::vector<const IR::Expression*> slices;

    convertConcatToList(slices, expr);
    return slices;
}

}  // namespace BFN
