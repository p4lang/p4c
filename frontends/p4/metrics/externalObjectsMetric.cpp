#include "frontends/p4/metrics/externalObjectsMetric.h"

namespace P4 {

void ExternalObjectsMetricPass::postorder(const IR::Type_Extern *node) {
    externTypeNames.insert(node->name.name);
    metrics.externStructures++;

    for (const auto &method : node->methods) {
        externMethods[node->name.name].insert(method->name.name);
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Declaration_Instance *node) {
    const IR::Type *type = node->type;
    cstring typeName;

    if (auto tn = type->to<IR::Type_Name>()) {
        typeName = tn->path->name.name;
        if (externTypeNames.count(typeName)) {
            metrics.externStructUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Member *node) {
    auto baseType = node->expr->type;

    if (baseType && baseType->is<IR::Type_Extern>()) {
        auto externType = baseType->to<IR::Type_Extern>();
        cstring externName = externType->name.name;
        cstring memberName = node->member.name;

        metrics.externStructUses++;
        // Check if member is a method call and count it.
        if (externMethods.count(externName) && externMethods[externName].count(memberName)) {
            metrics.externFunctionUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Method *node) {
    // Do not add methods that belong to an extern structure.
    if (!findContext<IR::Type_Extern>()) {
        metrics.externFunctions++;
        externFunctions.insert(node->name.name);
    }
}

void ExternalObjectsMetricPass::postorder(const IR::MethodCallExpression *node) {
    auto method = node->method;

    if (auto path = method->to<IR::PathExpression>()) {
        cstring calledName = path->path->name.name;
        if (externFunctions.count(calledName)) {
            metrics.externFunctionUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::P4Program * /*node*/) {
    if (!LOGGING(3)) return;

    std::cout << "Extern Functions (" << externFunctions.size() << "):\n";
    for (const auto &fn : externFunctions) {
        std::cout << " - " << fn << std::endl;
    }

    std::cout << "\nExtern Types and Methods per Type:\n";
    for (const auto &[typeName, methods] : externMethods) {
        std::cout << "  " << typeName << " (" << methods.size() << "):\n";
        for (const auto &method : methods) {
            std::cout << "   - " << method << std::endl;
        }
    }
}

}  // namespace P4
