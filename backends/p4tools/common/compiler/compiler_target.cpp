// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/compiler/compiler_target.h"

#include <string>

#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/core/target.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"
#include "midend/midEndLast.h"

namespace P4::P4Tools {

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName) {
    auto program = P4Tools::CompilerTarget::runParser(options);
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(options, toolName, program);
}

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName,
                                                  const std::string &source) {
#ifdef SUPPORT_P4_14
    auto program = parseP4String(source, options.langVersion);
#else
    auto program = parseP4String(source);
#endif
    if (program == nullptr) {
        return std::nullopt;
    }

    return runCompiler(options, toolName, program);
}

CompilerResultOrError CompilerTarget::runCompiler(const CompilerOptions &options,
                                                  std::string_view toolName,
                                                  IR::Ptr<IR::P4Program> program) {
    return get(toolName).runCompilerImpl(options, program);
}

CompilerResultOrError CompilerTarget::runCompilerImpl(const CompilerOptions &options,
                                                      IR::Ptr<IR::P4Program> program) const {
    program = runFrontend(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    program = runMidEnd(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    return std::make_shared<CompilerResult>(*program);
}

IR::Ptr<IR::P4Program> CompilerTarget::runParser(const ParserOptions &options) {
    auto program = parseP4File(options);
    if (errorCount() > 0) {
        return nullptr;
    }
    return program;
}

IR::Ptr<IR::P4Program> CompilerTarget::runFrontend(const CompilerOptions &options,
                                                   IR::Ptr<IR::P4Program> program) const {
    P4COptionPragmaParser optionsPragmaParser(false);
    program->apply(ApplyOptionsPragmas(optionsPragmaParser));

    auto frontEnd = mkFrontEnd();
    frontEnd.addDebugHook(options.getDebugHook());
    program = frontEnd.run(options, program);
    ReferenceMap refMap;
    TypeMap typeMap;
    // Perform a last round of type checking.
    program = program->apply(TypeChecking(&refMap, &typeMap, true));
    if ((program == nullptr) || errorCount() > 0) {
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

IR::Ptr<IR::P4Program> CompilerTarget::runMidEnd(const CompilerOptions &options,
                                                 IR::Ptr<IR::P4Program> program) const {
    auto midEnd = mkMidEnd(options);
    midEnd.addPasses({
        new P4::MidEndLast(),
    });
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
