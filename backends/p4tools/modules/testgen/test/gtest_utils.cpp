#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

#include <optional>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/variables.h"
#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "lib/compile_context.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/register.h"

namespace Test {

P4ToolsTestCase::P4ToolsTestCase(const P4Tools::CompilerResult &compilerResults)
    : compilerResults(compilerResults) {}

std::optional<const P4ToolsTestCase> P4ToolsTestCase::create(
    std::string deviceName, std::string archName, CompilerOptions::FrontendVersion langVersion,
    const std::string &source) {
    // Initialize the target.
    ensureInit();
    BUG_CHECK(P4Tools::Target::init(deviceName, archName), "Target %1%/%2% not supported",
              deviceName, archName);

    // Set up the compilation context and set the source language.
    AutoCompileContext autoCompileContext(P4Tools::CompilerTarget::makeContext());
    P4CContext::get().options().langVersion = langVersion;

    auto compilerResults = P4Tools::CompilerTarget::runCompiler(source);
    if (!compilerResults.has_value()) {
        return std::nullopt;
    }
    return P4ToolsTestCase(compilerResults.value());
}

const IR::P4Program &P4ToolsTestCase::getProgram() const {
    return getCompilerResult().getProgram();
}

const P4Tools::CompilerResult &P4ToolsTestCase::getCompilerResult() const {
    return compilerResults;
}

std::optional<const P4ToolsTestCase> P4ToolsTestCase::create_14(std::string deviceName,
                                                                std::string archName,
                                                                const std::string &source) {
    return create(deviceName, archName, CompilerOptions::FrontendVersion::P4_14, source);
}

std::optional<const P4ToolsTestCase> P4ToolsTestCase::create_16(std::string deviceName,
                                                                std::string archName,
                                                                const std::string &source) {
    return create(deviceName, archName, CompilerOptions::FrontendVersion::P4_16, source);
}

void P4ToolsTestCase::ensureInit() {
    static bool INITIALIZED = false;
    if (INITIALIZED) {
        return;
    }
    // Register supported compiler targets.
    P4Tools::P4Testgen::registerCompilerTargets();

    // Register supported Testgen targets.
    P4Tools::P4Testgen::registerTestgenTargets();

    INITIALIZED = true;
}

const IR::SymbolicVariable *SymbolicConverter::preorder(IR::Member *member) {
    return P4Tools::ToolsVariables::getSymbolicVariable(member->type, member->toString());
}

}  // namespace Test
