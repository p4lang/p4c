#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"

#include <cstdio>
#include <optional>

#include "ir/ir.h"
#include "lib/error.h"

namespace P4Tools::P4Testgen::Bmv2 {

bool MapDirectExterns::preorder(const IR::Declaration_Instance *declInstance) {
    declaredExterns[declInstance->name] = declInstance;
    return true;
}

std::optional<const IR::Declaration_Instance *> MapDirectExterns::getExternFromTableImplementation(
    const IR::Property *tableImplementation) {
    const auto *selectorExpr = tableImplementation->value->to<IR::ExpressionValue>();
    if (selectorExpr == nullptr) {
        ::error("Extern property is not an expression %1%", tableImplementation->value);
        return std::nullopt;
    }
    // If the extern expression is a constructor call it is not relevant.
    if (selectorExpr->expression->is<IR::ConstructorCallExpression>()) {
        return std::nullopt;
    }
    // Try to find the extern in the declared instances.
    const auto *implPath = selectorExpr->expression->to<IR::PathExpression>();
    if (implPath == nullptr) {
        ::error("Invalid extern path %1%", selectorExpr->expression);
        return std::nullopt;
    }
    // If the extern is not in the list of declared instances, move on.
    auto it = declaredExterns.find(implPath->path->name.name);
    if (it == declaredExterns.end()) {
        ::error("Cannot find direct extern declaration %1%", implPath);
        return std::nullopt;
    }
    // BMv2 does not support direct externs attached to multiple tables.
    auto mappedTable = directExternMap.find(it->second->controlPlaneName());
    if (mappedTable != directExternMap.end()) {
        ::error(
            "Direct extern %1% was already mapped to table %2%. It can not be "
            "attached to two tables.",
            it->second, mappedTable->second);
        return std::nullopt;
    }
    return it->second;
}

bool MapDirectExterns::preorder(const IR::P4Table *table) {
    // Try to get extern implementation properties from the table.
    for (const auto *tableProperty : kTableExternProperties) {
        const auto *impl = table->properties->getProperty(tableProperty);
        if (impl != nullptr) {
            auto declInstanceOpt = getExternFromTableImplementation(impl);
            if (declInstanceOpt.has_value()) {
                directExternMap.emplace(declInstanceOpt.value()->controlPlaneName(), table);
            }
        }
    }
    return false;
}

const DirectExternMap &MapDirectExterns::getDirectExternMap() { return directExternMap; }

}  // namespace P4Tools::P4Testgen::Bmv2
