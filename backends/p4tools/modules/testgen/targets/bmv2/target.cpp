#include "backends/p4tools/modules/testgen/targets/bmv2/target.h"

#include <cstddef>
#include <map>
#include <vector>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/compiler_result.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4runtime_translation.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend.h"

namespace P4C::P4Tools::P4Testgen::Bmv2 {

/* =============================================================================================
 *  Bmv2V1ModelTestgenTarget implementation
 * ============================================================================================= */

Bmv2V1ModelTestgenTarget::Bmv2V1ModelTestgenTarget() : TestgenTarget("bmv2", "v1model") {}

void Bmv2V1ModelTestgenTarget::make() {
    static Bmv2V1ModelTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2V1ModelTestgenTarget();
    }
}

CompilerResultOrError Bmv2V1ModelTestgenTarget::runCompilerImpl(
    const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    /// After the front end, get the P4Runtime API for the V1model architecture.
    auto p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "v1model"_cs);

    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }

    program = runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    // Create DCG.
    NodesCallGraph *dcg = nullptr;
    if (TestgenOptions::get().dcg || !TestgenOptions::get().pattern.empty()) {
        dcg = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(dcg);
        program->apply(dcgCreator);
    }
    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }
    /// Collect coverage information about the program.
    auto coverage = P4::Coverage::CollectNodes(TestgenOptions::get().coverageOptions);
    program->apply(coverage);
    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }

    // Parses any @refers_to annotations and converts them into a vector of restrictions.
    auto refersToParser = RefersToParser();
    program->apply(refersToParser);
    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }
    ConstraintsVector p4ConstraintsRestrictions = refersToParser.getRestrictionsVector();

    // Defines all "entry_restriction" and then converts restrictions from string to IR
    // expressions, and stores them in p4ConstraintsRestrictions to move targetConstraints
    // further.
    program->apply(AssertsParser(p4ConstraintsRestrictions));
    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }
    // Try to map all instances of direct externs to the table they are attached to.
    // Save the map in @var directExternMap.
    auto directExternMapper = MapDirectExterns();
    program->apply(directExternMapper);
    if (::P4C::errorCount() > 0) {
        return std::nullopt;
    }

    return {*new BMv2V1ModelCompilerResult{
        TestgenCompilerResult(CompilerResult(*program), coverage.getCoverableNodes(), dcg),
        p4runtimeApi, directExternMapper.getDirectExternMap(), p4ConstraintsRestrictions}};
}

MidEnd Bmv2V1ModelTestgenTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Parse BMv2-specific annotations.
        new BMV2::ParseAnnotations(),
        new P4::TypeChecking(refMap, typeMap, true),
        new PropagateP4RuntimeTranslation(*typeMap),
    });
    midEnd.addDefaultPasses();

    return midEnd;
}

const Bmv2V1ModelProgramInfo *Bmv2V1ModelTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 6, "%1%: The BMV2 architecture requires 6 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = Bmv2V1ModelProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);

        if (idx < 3) {
            declIdToGress[declType->declid] = BMV2_INGRESS;
        } else {
            declIdToGress[declType->declid] = BMV2_EGRESS;
        }
    }

    return new Bmv2V1ModelProgramInfo(*compilerResult.checkedTo<BMv2V1ModelCompilerResult>(),
                                      programmableBlocks, declIdToGress);
}

Bmv2TestBackend *Bmv2V1ModelTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
    SymbolicExecutor &symbex) const {
    return new Bmv2TestBackend(*programInfo.checkedTo<Bmv2V1ModelProgramInfo>(),
                               testBackendConfiguration, symbex);
}

Bmv2V1ModelCmdStepper *Bmv2V1ModelTestgenTarget::getCmdStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelCmdStepper(state, solver, programInfo);
}

Bmv2V1ModelExprStepper *Bmv2V1ModelTestgenTarget::getExprStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelExprStepper(state, solver, programInfo);
}

}  // namespace P4C::P4Tools::P4Testgen::Bmv2
