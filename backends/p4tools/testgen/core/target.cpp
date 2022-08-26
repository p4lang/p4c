#include "backends/p4tools/testgen/core/target.h"

#include <string>
#include <utility>

#include "backends/p4tools/common/core/target.h"
#include "ir/ir.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

ArchSpec::ArchSpec(cstring packageName, const std::vector<ArchMember>& archVectorInput)
    : packageName(packageName) {
    for (size_t idx = 0; idx < archVectorInput.size(); ++idx) {
        auto archMember = archVectorInput.at(idx);
        archVector.emplace_back(archMember);
        auto blockName = archMember.blockName;
        blockIndices[blockName] = idx;
    }
}

size_t ArchSpec::getBlockIndex(cstring blockName) const {
    auto blockIndex = blockIndices.find(blockName);
    if (blockIndex != blockIndices.end()) {
        return blockIndex->second;
    }
    BUG("Unable to find block %s in the architecture map.", blockName);
}

cstring ArchSpec::getParamName(cstring blockName, size_t paramIndex) const {
    auto blockIndex = getBlockIndex(blockName);
    auto params = archVector.at(blockIndex).blockParams;
    BUG_CHECK(paramIndex < params.size(), "Param index %s out of range. Vector size: %s",
              paramIndex, params.size());
    return params.at(paramIndex);
}

cstring ArchSpec::getParamName(size_t blockIndex, size_t paramIndex) const {
    BUG_CHECK(blockIndex < archVector.size(), "Block index %s out of range. Vector size: %s",
              blockIndex, archVector.size());
    auto params = archVector.at(blockIndex).blockParams;
    BUG_CHECK(paramIndex < params.size(), "Param index %s out of range. Vector size: %s",
              paramIndex, params.size());
    return params.at(paramIndex);
}

const ArchSpec::ArchMember* ArchSpec::getArchMember(size_t blockIndex) const {
    BUG_CHECK(blockIndex < archVector.size(), "Index %s out of range. Vector size: %s", blockIndex,
              archVector.size());
    return &archVector.at(blockIndex);
}

size_t ArchSpec::getArchVectorSize() const { return archVector.size(); }

cstring ArchSpec::getPackageName() const { return packageName; }

TestgenTarget::TestgenTarget(std::string deviceName, std::string archName)
    : Target("testgen", std::move(deviceName), std::move(archName)) {}

const ProgramInfo* TestgenTarget::initProgram_impl(const IR::P4Program* program) const {
    // Check that the program has at least one main declaration.
    const auto mainCount = program->getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version of
    // initProgram_impl that takes the main declaration.
    const auto* mainIDecl = program->getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program->main);

    const auto* mainNode = mainIDecl->getNode();
    const auto* mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return initProgram_impl(program, mainDecl);
}

const TestgenTarget& TestgenTarget::get() { return Target::get<TestgenTarget>("testgen"); }

void TestgenTarget::argumentsToTypeDeclarations(
    const NamespaceContext* ns, const IR::Vector<IR::Argument>* inputArgs,
    std::vector<const IR::Type_Declaration*>& resultDecls) {
    for (const auto* arg : *inputArgs) {
        const auto* expr = arg->expression;

        const IR::Type_Declaration* declType = nullptr;

        if (const auto* ctorCall = expr->to<IR::ConstructorCallExpression>()) {
            const auto* constructedTypeName = ctorCall->constructedType->checkedTo<IR::Type_Name>();

            // Find the corresponding type declaration in the namespace.
            declType = ns->findDecl(constructedTypeName->path)->checkedTo<IR::Type_Declaration>();
        } else if (const auto* pathExpr = expr->to<IR::PathExpression>()) {
            // Look up the path expression in the declaration map, and expect to find a
            // declaration instance.
            const auto* declInstance =
                ns->findDecl(pathExpr->path)->checkedTo<IR::Declaration_Instance>();
            declType = declInstance->type->checkedTo<IR::Type_Declaration>();
        } else {
            BUG("Unexpected main-declaration argument node type: %1%", expr->node_type_name());
        }

        // The constructor's parameter list should be empty, since the compiler should have
        // substituted the constructor arguments for us.
        if (const auto* iApply = declType->to<IR::IContainer>()) {
            const IR::ParameterList* ctorParams = iApply->getConstructorParameters();
            BUG_CHECK(ctorParams->empty(), "Compiler did not eliminate constructor parameters: %1%",
                      ctorParams);
        } else {
            BUG("Does not instantiate an IContainer: %1%", expr);
        }

        resultDecls.emplace_back(declType);
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools
