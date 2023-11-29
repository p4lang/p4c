#include "backends/p4tools/modules/testgen/lib/util.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"

namespace P4Tools::P4Testgen {

std::vector<const IR::Type_Declaration *> argumentsToTypeDeclarations(
    const IR::IGeneralNamespace *ns, const IR::Vector<IR::Argument> *inputArgs) {
    std::vector<const IR::Type_Declaration *> resultDecls;
    for (const auto *arg : *inputArgs) {
        const auto *expr = arg->expression;

        const IR::Type_Declaration *declType = nullptr;

        if (const auto *ctorCall = expr->to<IR::ConstructorCallExpression>()) {
            const auto *constructedTypeName = ctorCall->constructedType->checkedTo<IR::Type_Name>();

            // Find the corresponding type declaration in the top-level namespace.
            declType = ProgramInfo::findProgramDecl(ns, constructedTypeName->path)
                           ->checkedTo<IR::Type_Declaration>();
        } else if (const auto *pathExpr = expr->to<IR::PathExpression>()) {
            // Look up the path expression in the top-level namespace and expect to find a
            // declaration instance.
            const auto *declInstance = ProgramInfo::findProgramDecl(ns, pathExpr->path)
                                           ->checkedTo<IR::Declaration_Instance>();
            declType = declInstance->type->checkedTo<IR::Type_Declaration>();
        } else {
            BUG("Unexpected main-declaration argument node type: %1%", expr->node_type_name());
        }

        // The constructor's parameter list should be empty, since the compiler should have
        // substituted the constructor arguments for us.
        if (const auto *iApply = declType->to<IR::IContainer>()) {
            const IR::ParameterList *ctorParams = iApply->getConstructorParameters();
            BUG_CHECK(ctorParams->empty(), "Compiler did not eliminate constructor parameters: %1%",
                      ctorParams);
        } else {
            BUG("Does not instantiate an IContainer: %1%", expr);
        }

        resultDecls.emplace_back(declType);
    }
    return resultDecls;
}

}  // namespace P4Tools::P4Testgen
