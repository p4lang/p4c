#include "backends/p4tools/modules/rtsmith/core/target.h"

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "backends/p4tools/modules/rtsmith/options.h"
#include "backends/p4tools/modules/rtsmith/toolname.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4::P4Tools::RtSmith {

RtSmithTarget::RtSmithTarget(const std::string &deviceName, const std::string &archName)
    : CompilerTarget(TOOL_NAME, deviceName, archName) {}

const ProgramInfo *RtSmithTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions) const {
    const auto &program = compilerResult.getProgram();
    // Check that the program has at least one main declaration.
    const auto mainCount = program.getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version
    // of produceProgramInfoImpl that takes the main declaration.
    const auto *mainIDecl = program.getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program.main);

    const auto *mainNode = mainIDecl->getNode();
    const auto *mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return produceProgramInfoImpl(compilerResult, rtSmithOptions, mainDecl);
}

RuntimeFuzzer &RtSmithTarget::getFuzzer(const ProgramInfo &programInfo) {
    return get().getFuzzerImpl(programInfo);
}

const RtSmithTarget &RtSmithTarget::get() { return Target::get<RtSmithTarget>(TOOL_NAME); }

const ProgramInfo *RtSmithTarget::produceProgramInfo(const CompilerResult &compilerResult,
                                                     const RtSmithOptions &rtSmithOptions) {
    return get().produceProgramInfoImpl(compilerResult, rtSmithOptions);
}

ICompileContext *RtSmithTarget::makeContext() const {
    return new P4Tools::CompileContext<RtSmithOptions>();
}
}  // namespace P4::P4Tools::RtSmith
