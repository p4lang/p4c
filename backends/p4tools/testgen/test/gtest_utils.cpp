#include "backends/p4tools/testgen/test/gtest_utils.h"

#include <boost/none.hpp>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "lib/compile_context.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen/register.h"

namespace Test {

boost::optional<const P4ToolsTestCase> P4ToolsTestCase::create(
    std::string deviceName, std::string archName, CompilerOptions::FrontendVersion langVersion,
    const std::string& source) {
    // Initialize the target.
    ensureInit();
    BUG_CHECK(P4Tools::Target::init(deviceName, archName), "Target %1%/%2% not supported",
              deviceName, archName);

    // Set up the compilation context and set the source language.
    AutoCompileContext autoCompileContext(P4Tools::CompilerTarget::makeContext());
    P4CContext::get().options().langVersion = langVersion;

    auto program = P4Tools::CompilerTarget::runCompiler(source);
    if (!program) {
        return boost::none;
    }
    return P4ToolsTestCase{*program};
}

boost::optional<const P4ToolsTestCase> P4ToolsTestCase::create_14(std::string deviceName,
                                                                  std::string archName,
                                                                  const std::string& source) {
    return create(deviceName, archName, CompilerOptions::FrontendVersion::P4_14, source);
}

boost::optional<const P4ToolsTestCase> P4ToolsTestCase::create_16(std::string deviceName,
                                                                  std::string archName,
                                                                  const std::string& source) {
    return create(deviceName, archName, CompilerOptions::FrontendVersion::P4_16, source);
}

void P4ToolsTestCase::ensureInit() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    // Register supported compiler targets.
    P4Tools::P4Testgen::registerCompilerTargets();

    // Register supported Testgen targets.
    P4Tools::P4Testgen::registerTestgenTargets();

    initialized = true;
}

}  // namespace Test
