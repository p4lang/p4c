#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"

#include <utility>

#include "ir/declaration.h"
#include "ir/id.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::P4Testgen::Bmv2 {

bool MapDirectExterns::preorder(const IR::Declaration_Instance *declInstance) {
    declaredExterns[declInstance->name] = declInstance;
    return true;
}

bool MapDirectExterns::preorder(const IR::P4Table *table) {
    // Try to either get counters or meters from the table.
    const auto *impl = table->properties->getProperty("meters");
    if (impl == nullptr) {
        impl = table->properties->getProperty("counters");
    }
    if (impl == nullptr) {
        return false;
    }
    // Cannot map a temporary mapped direct extern without a counter.
    const auto *selectorExpr = impl->value->checkedTo<IR::ExpressionValue>();
    if (selectorExpr->expression->is<IR::ConstructorCallExpression>()) {
        return true;
    }
    // Try to find the extern in the declared instances.
    const auto *implPath = selectorExpr->expression->to<IR::PathExpression>();
    auto implementationLabel = implPath->path->name.name;

    // If the extern is not in the list of declared instances, move on.
    auto it = declaredExterns.find(implementationLabel);
    if (it == declaredExterns.end()) {
        return true;
    }
    // BMv2 does not support direct externs attached to multiple tables.
    const auto *declInstance = it->second;
    if (directExternMap.find(declInstance) != directExternMap.end()) {
        FATAL_ERROR(
            "Direct extern was already mapped to a table. It can not be attached to two tables. ");
        return true;
    }
    directExternMap.emplace(declInstance, table);
    return true;
}

const std::map<const IR::IDeclaration *, const IR::P4Table *>
    &MapDirectExterns::getdirectExternMap() {
    return directExternMap;
}

}  // namespace P4Tools::P4Testgen::Bmv2
