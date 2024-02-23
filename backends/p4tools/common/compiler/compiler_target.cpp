#include "backends/p4tools/common/compiler/compiler_target.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/core/target.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "lib/compile_context.h"
#include "lib/error.h"

namespace P4Tools {

const IR::P4Program &CompilerResult::getProgram() const { return program; }

CompilerResult::CompilerResult(const IR::P4Program &program) : program(program) {}

ICompileContext *CompilerTarget::makeContext() { return get().makeContextImpl(); }

std::vector<const char *> *CompilerTarget::initCompiler(int argc, char **argv) {
    return get().initCompilerImpl(argc, argv);
}

CompilerResultOrError CompilerTarget::runCompiler() {
    const auto *program = P4Tools::CompilerTarget::runParser();
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(program);
}

CompilerResultOrError CompilerTarget::runCompiler(const std::string &source) {
    const auto *program = P4::parseP4String(source, P4CContext::get().options().langVersion);
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(program);
}

CompilerResultOrError CompilerTarget::runCompiler(const IR::P4Program *program) {
    return get().runCompilerImpl(program);
}

CompilerResultOrError CompilerTarget::runCompilerImpl(const IR::P4Program *program) const {
    const auto &self = get();

    program = self.runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    program = self.runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    return *new CompilerResult(*program);
}

ICompileContext *CompilerTarget::makeContextImpl() const {
    return new CompileContext<CompilerOptions>();
}

std::vector<const char *> *CompilerTarget::initCompilerImpl(int argc, char **argv) const {
    auto *result = P4CContext::get().options().process(argc, argv);
    return ::errorCount() > 0 ? nullptr : result;
}

const IR::P4Program *CompilerTarget::runParser() {
    auto &options = P4CContext::get().options();

    const auto *program = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return nullptr;
    }
    return program;
}

const IR::P4Program *CompilerTarget::runFrontend(const IR::P4Program *program) const {
    // Dynamic cast to get the CompilerOptions from ParserOptions
    auto &options = dynamic_cast<CompilerOptions &>(P4CContext::get().options());

    P4::P4COptionPragmaParser optionsPragmaParser;
    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

    auto frontEnd = mkFrontEnd();
    frontEnd.addDebugHook(options.getDebugHook());
    program = frontEnd.run(options, program);
    if ((program == nullptr) || ::errorCount() > 0) {
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

const IR::P4Program *CompilerTarget::runMidEnd(const IR::P4Program *program) const {
    // Dynamic cast to get the CompilerOptions from ParserOptions
    auto &options = dynamic_cast<CompilerOptions &>(P4CContext::get().options());

    auto midEnd = mkMidEnd(options);
    midEnd.addDebugHook(options.getDebugHook(), true);
    return program->apply(midEnd);
}

CompilerTarget::CompilerTarget(std::string deviceName, std::string archName)
    : Target("compiler", std::move(deviceName), std::move(archName)) {}

const CompilerTarget &CompilerTarget::get() { return Target::get<CompilerTarget>("compiler"); }
}  // namespace P4Tools
