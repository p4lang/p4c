#include "backends/p4tools/common/compiler/compiler_target.h"

#include <string>
#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/compiler/configuration.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "p4tools/common/core/target.h"

namespace P4Tools {

ICompileContext* CompilerTarget::makeContext() { return get().makeContext_impl(); }

std::vector<const char*>* CompilerTarget::initCompiler(int argc, char** argv) {
    return get().initCompiler_impl(argc, argv);
}

boost::optional<const IR::P4Program*> CompilerTarget::runCompiler() {
    const auto* program = P4Tools::CompilerTarget::runParser();
    if (program == nullptr) {
        return boost::none;
    }

    return runCompiler(program);
}

boost::optional<const IR::P4Program*> CompilerTarget::runCompiler(const std::string& source) {
    const auto* program = P4::parseP4String(source, P4CContext::get().options().langVersion);
    if (program == nullptr) {
        return boost::none;
    }

    return runCompiler(program);
}

boost::optional<const IR::P4Program*> CompilerTarget::runCompiler(const IR::P4Program* program) {
    const auto& self = get();

    program = self.runFrontend(program);
    if (program == nullptr) {
        return boost::none;
    }

    program = self.runMidEnd(program);
    if (program == nullptr) {
        return boost::none;
    }

    // Rewrite all occurrences of ArrayIndex to be members instead.
    // IMPORTANT: After this change, the program will no longer type-check.
    // This is why perform this rewrite after all front and mid end passes have been applied.
    program = program->apply(HSIndexToMember());

    return program;
}

ICompileContext* CompilerTarget::makeContext_impl() const {
    return new CompileContext<CompilerOptions>();
}

std::vector<const char*>* CompilerTarget::initCompiler_impl(int argc, char** argv) const {
    auto* result = P4CContext::get().options().process(argc, argv);
    return ::errorCount() > 0 ? nullptr : result;
}

const IR::P4Program* CompilerTarget::runParser() {
    auto& options = P4CContext::get().options();

    const auto* program = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return nullptr;
    }
    return program;
}

const IR::P4Program* CompilerTarget::runFrontend(const IR::P4Program* program) const {
    // Dynamic cast to get the CompilerOptions from ParserOptions
    auto& options = dynamic_cast<CompilerOptions&>(P4CContext::get().options());

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

MidEnd CompilerTarget::mkMidEnd(const CompilerOptions& options) const {
    MidEnd midEnd(options);
    midEnd.addDefaultPasses();
    return midEnd;
}

const IR::P4Program* CompilerTarget::runMidEnd(const IR::P4Program* program) const {
    // Dynamic cast to get the CompilerOptions from ParserOptions
    auto& options = dynamic_cast<CompilerOptions&>(P4CContext::get().options());

    auto midEnd = mkMidEnd(options);
    midEnd.addDebugHook(options.getDebugHook(), true);
    return program->apply(midEnd);
}

CompilerTarget::CompilerTarget(std::string deviceName, std::string archName)
    : Target("compiler", deviceName, archName) {}

const CompilerTarget& CompilerTarget::get() { return Target::get<CompilerTarget>("compiler"); }

}  // namespace P4Tools
