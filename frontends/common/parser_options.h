/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/* -*-C++-*- */

#ifndef FRONTENDS_COMMON_PARSER_OPTIONS_H_
#define FRONTENDS_COMMON_PARSER_OPTIONS_H_

#include <stdio.h>

#include <set>
#include <vector>

#include "ir/configuration.h"
#include "ir/gen-tree-macro.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/error_reporter.h"
#include "lib/options.h"

// Standard include paths for .p4 header files. The values are determined by
// `configure`.
extern const char *p4includePath;
extern const char *p4_14includePath;

// Base class for compiler options.
// This class contains the options for the front-ends.
// Each back-end should subclass this file.
class ParserOptions : public Util::Options {
    bool close_input = false;
    static const char *defaultMessage;

    // annotation names that are to be ignored by the compiler
    std::set<cstring> disabledAnnotations;

 protected:
    // Function that is returned by getDebugHook.
    void dumpPass(const char *manager, unsigned seq, const char *pass, const IR::Node *node) const;
    // Checks if parsed options make sense with respect to each-other.
    virtual void validateOptions() const;

 public:
    ParserOptions();
    std::vector<const char *> *process(int argc, char *const argv[]) override;
    enum class FrontendVersion { P4_14, P4_16 };
    // Name of executable that is being run.
    cstring exe_name;
    // Which language to compile
    FrontendVersion langVersion = FrontendVersion::P4_16;
    // options to pass to preprocessor
    cstring preprocessor_options = "";
    // file to compile (- for stdin)
    cstring file = nullptr;
    // if true preprocess only
    bool doNotCompile = false;
    // Compiler version.
    cstring compilerVersion;
    // if true skip preprocess
    bool doNotPreprocess = false;
    // substrings matched against pass names
    std::vector<cstring> top4;
    // debugging dumps of programs written in this folder
    cstring dumpFolder = ".";
    // If false, optimization of callee parsers (subparsers) inlining is disabled.
    bool optimizeParserInlining = false;
    // Expect that the only remaining argument is the input file.
    void setInputFile();
    // Return target specific include path.
    const char *getIncludePath() override;
    // Returns the output of the preprocessor.
    FILE *preprocess();
    // Closes the input stream returned by preprocess.
    void closeInput(FILE *input) const;
    // True if we are compiling a P4 v1.0 or v1.1 program
    bool isv1() const;
    // Get a debug hook function suitable for insertion
    // in the pass managers that are executed.
    DebugHook getDebugHook() const;
    // Check whether this particular annotation was disabled
    bool isAnnotationDisabled(const IR::Annotation *a) const;
    // Search and set 'includePathOut' to be the first valid path from the
    // list of possible relative paths.
    bool searchForIncludePath(const char *&includePathOut, std::vector<cstring> relativePaths,
                              const char *);
    /// If true do not generate #include statements.
    /// Used for debugging.
    bool noIncludes = false;
};

/// A compilation context which exposes compiler options and a compiler
/// configuration.
class P4CContext : public BaseCompileContext {
 public:
    /// @return the current compilation context, which must inherit from
    /// P4CContext.
    static P4CContext &get();

    /// @return the compiler configuration for the current compilation context.
    /// If there is no current compilation context, the default configuration is
    /// returned.
    static const P4CConfiguration &getConfig();

    P4CContext() {}

    /// @return the compiler options for this compilation context.
    virtual ParserOptions &options() = 0;

    /// @return the default diagnostic action for calls to `::warning()`.
    DiagnosticAction getDefaultWarningDiagnosticAction() final {
        return errorReporter().getDefaultWarningDiagnosticAction();
    }

    /// set the default diagnostic action for calls to `::warning()`.
    void setDefaultWarningDiagnosticAction(DiagnosticAction action) {
        errorReporter().setDefaultWarningDiagnosticAction(action);
    }

    /// @return the action to take for the given diagnostic, falling back to the
    /// default action if it wasn't overridden via the command line or a pragma.
    DiagnosticAction getDiagnosticAction(cstring diagnostic, DiagnosticAction defaultAction) final {
        return errorReporter().getDiagnosticAction(diagnostic, defaultAction);
    }

    /// Set the action to take for the given diagnostic.
    void setDiagnosticAction(cstring diagnostic, DiagnosticAction action) {
        errorReporter().setDiagnosticAction(diagnostic, action);
    }

 protected:
    /// @return true if the given diagnostic is known to be valid. This is
    /// intended to help the user find misspelled diagnostics and the like; it
    /// doesn't affect functionality.
    virtual bool isRecognizedDiagnostic(cstring diagnostic);

    /// @return the compiler configuration associated with this type of
    /// compilation context.
    virtual const P4CConfiguration &getConfigImpl();
};

/// A utility template which can be used to easily make subclasses of P4CContext
/// which expose a particular subclass of CompilerOptions. This is provided as a
/// convenience since this is all many backends need.
template <typename OptionsType>
class P4CContextWithOptions final : public P4CContext {
 public:
    /// @return the current compilation context, which must be of type
    /// P4CContextWithOptions<OptionsType>.
    static P4CContextWithOptions &get() {
        return CompileContextStack::top<P4CContextWithOptions>();
    }

    P4CContextWithOptions() {}

    template <typename OptionsDerivedType>
    P4CContextWithOptions(P4CContextWithOptions<OptionsDerivedType> &context) {
        optionsInstance = context.options();
    }

    /// @return the compiler options for this compilation context.
    OptionsType &options() override { return optionsInstance; }

 private:
    /// Compiler options for this compilation context.
    OptionsType optionsInstance;
};

#endif /* FRONTENDS_COMMON_PARSER_OPTIONS_H_*/
