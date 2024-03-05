#include "backends/p4tools/modules/testgen/core/target.h"

#include <string>
#include <utility>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"

namespace P4Tools::P4Testgen {

TestgenTarget::TestgenTarget(std::string deviceName, std::string archName)
    : Target("testgen", std::move(deviceName), std::move(archName)) {}

const ProgramInfo *TestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult) const {
    const auto &program = compilerResult.getProgram();
    // Check that the program has at least one main declaration.
    const auto mainCount = program.getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version of
    // produceProgramInfoImpl that takes the main declaration.
    const auto *mainIDecl = program.getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program.main);

    const auto *mainNode = mainIDecl->getNode();
    const auto *mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return produceProgramInfoImpl(compilerResult, mainDecl);
}

const TestgenTarget &TestgenTarget::get() { return Target::get<TestgenTarget>("testgen"); }

TestBackEnd *TestgenTarget::getTestBackend(const ProgramInfo &programInfo,
                                           const TestBackendConfiguration &testBackendConfiguration,
                                           SymbolicExecutor &symbex) {
    return get().getTestBackendImpl(programInfo, testBackendConfiguration, symbex);
}

const ProgramInfo *TestgenTarget::produceProgramInfo(const CompilerResult &compilerResult) {
    return get().produceProgramInfoImpl(compilerResult);
}

ExprStepper *TestgenTarget::getExprStepper(ExecutionState &state, AbstractSolver &solver,
                                           const ProgramInfo &programInfo) {
    return get().getExprStepperImpl(state, solver, programInfo);
}

CmdStepper *TestgenTarget::getCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                         const ProgramInfo &programInfo) {
    return get().getCmdStepperImpl(state, solver, programInfo);
}

}  // namespace P4Tools::P4Testgen
