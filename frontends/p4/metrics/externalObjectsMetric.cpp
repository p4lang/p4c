#include "externalObjectsMetric.h"
#include <iostream>

namespace P4 {

void ExternalObjectsMetricPass::postorder(const IR::Type_Extern* node) {
    externTypeNames.insert(node->name.name.string());
    metrics.externMetrics.externStructures++;
    
    for (const auto& method : node->methods) {
        externMethods[node->name.name.string()].insert(method->name.name.string());
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Declaration_Instance* node) {
    const IR::Type* type = node->type;
    std::string typeName;

    if (auto tn = type->to<IR::Type_Name>()) {
        typeName = tn->path->name.name.string();
        if (externTypeNames.count(typeName)) {
            metrics.externMetrics.externStructUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Member* node) {
    auto baseType = node->expr->type;

    if (baseType && baseType->is<IR::Type_Extern>()) {
        auto externType = baseType->to<IR::Type_Extern>();
        std::string externName = externType->name.name.string();
        std::string memberName = node->member.name.string();

        metrics.externMetrics.externStructUses++;
        // Check if member is a method call and count it.
        if (externMethods.count(externName) && 
            externMethods[externName].count(memberName)) {
            metrics.externMetrics.externFunctionUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::Method* node) {
    if (!findContext<IR::Type_Extern>()) {
        metrics.externMetrics.externFunctions++;
        externFunctions.insert(node->name.name.string());
    }
}

void ExternalObjectsMetricPass::postorder(const IR::MethodCallExpression* node) {
    auto method = node->method;

    if (auto path = method->to<IR::PathExpression>()) {
        std::string calledName = path->path->name.name.string();
        if (externFunctions.count(calledName)) {
            metrics.externMetrics.externFunctionUses++;
        }
    }
}

void ExternalObjectsMetricPass::postorder(const IR::P4Program* /*node*/) {
    if (!LOGGING(3)) return;

    std::cout << "Extern Functions (" << externFunctions.size() << "):\n";
    for (const auto& fn : externFunctions) {
        std::cout << " - " << fn << std::endl;
    }

    std::cout << "\nExtern Types and Methods per Type:\n";
    for (const auto& [typeName, methods] : externMethods) {
        std::cout << "  " << typeName << " (" << methods.size() << "):\n";
        for (const auto& method : methods) {
            std::cout << "   - " << method << std::endl;
        }
    }
}


}  // namespace P4