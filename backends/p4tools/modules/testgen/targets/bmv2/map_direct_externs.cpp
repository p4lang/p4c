#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"

namespace P4Tools::P4Testgen::Bmv2 {

bool MapDirectExterns::preorder(const IR::Declaration_Instance *declInstance) {
    declaredExterns[declInstance->name] = declInstance;
    return true;
}

bool MapDirectExterns::preorder(const IR::P4Table *table) {
    const auto *impl = table->properties->getProperty("meters");
    if (impl == nullptr) {
        impl = table->properties->getProperty("counters");
    }
    if (impl == nullptr) {
        return false;
    }
    const auto *selectorExpr = impl->value->checkedTo<IR::ExpressionValue>();
    if (const auto *implCall = selectorExpr->expression->to<IR::ConstructorCallExpression>()) {
        return true;
    }
    const auto *implPath = selectorExpr->expression->to<IR::PathExpression>();
    auto implementationLabel = implPath->path->name.name;

    auto it = declaredExterns.find(implementationLabel);
    if (it == declaredExterns.end()) {
        return true;
    }
    const auto *declInstance = it->second;
    if (directExternMap.find(declInstance) != directExternMap.end()) {
        return true;
    }
    directExternMap.emplace(declInstance, table);

    return true;
}

std::map<const IR::IDeclaration *, const IR::P4Table *> MapDirectExterns::getdirectExternMap() {
    return directExternMap;
}

MapDirectExterns::MapDirectExterns() = default;

}  // namespace P4Tools::P4Testgen::Bmv2
