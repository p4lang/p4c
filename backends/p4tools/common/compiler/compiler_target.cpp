#include "backends/p4tools/common/compiler/compiler_target.h"

#include <string>

#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/core/target.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "lib/compile_context.h"
#include "lib/error.h"

namespace P4::P4Tools {

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName) {
    const auto *program = P4Tools::CompilerTarget::runParser(options);
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(options, toolName, program);
}

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName,
                                                  const std::string &source) {
    const auto *program = P4::parseP4String(source, options.langVersion);
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(options, toolName, program);
}

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName,
                                                  const IR::P4Program *program) {
    return get(toolName).runCompilerImpl(options, program);
}

CompilerResultOrError CompilerTarget::runCompilerImpl(const CompilerOptions &options,
                                                      const IR::P4Program *program) const {
    program = runFrontend(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    program = runMidEnd(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    return *new CompilerResult(*program);
}

const IR::P4Program *CompilerTarget::runParser(const ParserOptions &options) {
    const auto *program = P4::parseP4File(options);
    if (::P4::errorCount() > 0) {
        return nullptr;
    }
    return program;
}

const IR::P4Program *CompilerTarget::runFrontend(const CompilerOptions &options,
                                                 const IR::P4Program *program) const {
    P4::P4COptionPragmaParser optionsPragmaParser;
    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

    auto frontEnd = mkFrontEnd();
    frontEnd.addDebugHook(options.getDebugHook());
    program = frontEnd.run(options, program);
    if ((program == nullptr) || ::P4::errorCount() > 0) {
        return nullptr;
    }
    return program;
}

P4::FrontEnd CompilerTarget::mkFrontEnd() const { return {}; }

MidEnd CompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.addDefaultPasses();
    midEnd.setStopOnError(true);
    return midEnd;
}

const IR::P4Program *CompilerTarget::runMidEnd(const CompilerOptions &options,
                                               const IR::P4Program *program) const {
    auto midEnd = mkMidEnd(options);
    midEnd.addDebugHook(options.getDebugHook(), true);
    return program->apply(midEnd);
}

CompilerTarget::CompilerTarget(std::string_view toolName, const std::string &deviceName,
                               const std::string &archName)
    : Target(toolName, deviceName, archName) {}

const CompilerTarget &CompilerTarget::get(std::string_view toolName) {
    return Target::get<CompilerTarget>(toolName);
}

}  // namespace P4::P4Tools
