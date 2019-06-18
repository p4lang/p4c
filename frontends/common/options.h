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

#ifndef FRONTENDS_COMMON_OPTIONS_H_
#define FRONTENDS_COMMON_OPTIONS_H_

#include <unordered_map>
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/options.h"
#include "ir/ir.h"  // for DebugHook definition
// for p4::P4RuntimeFormat definition
#include "control-plane/p4RuntimeSerializer.h"

// Standard include paths for .p4 header files. The values are determined by
// `configure`.
extern const char* p4includePath;
extern const char* p4_14includePath;

// Base class for compiler options.
// This class contains the options for the front-ends.
// Each back-end should subclass this file.
class CompilerOptions : public Util::Options {
    bool close_input = false;
    static const char* defaultMessage;

    // Checks if parsed options make sense with respect to each-other.
    void validateOptions() const;

 protected:
    // Function that is returned by getDebugHook.
    void dumpPass(const char* manager, unsigned seq, const char* pass, const IR::Node* node) const;

 public:
    CompilerOptions();
    std::vector<const char*>* process(int argc, char* const argv[]) override;

    enum class FrontendVersion {
        P4_14,
        P4_16
    };

    // Name of executable that is being run.
    cstring exe_name;
    // Which language to compile
    FrontendVersion langVersion = FrontendVersion::P4_14;
    // options to pass to preprocessor
    cstring preprocessor_options = "";
    // file to compile (- for stdin)
    cstring file = nullptr;
    // if true preprocess only
    bool doNotCompile = false;
    // if true skip preprocess
    bool doNotPreprocess = false;
    // debugging dumps of programs written in this folder
    cstring dumpFolder = ".";
    // Pretty-print the program in the specified file
    cstring prettyPrintFile = nullptr;
    // Compiler version.
    cstring compilerVersion;

    // Dump a JSON representation of the IR in the file
    cstring dumpJsonFile = nullptr;

    // Dump and undump the IR tree
    bool debugJson = false;

    // Write a P4Runtime control plane API description to the specified file.
    cstring p4RuntimeFile = nullptr;

    // Write static table entries as a P4Runtime WriteRequest message to the specified file.
    cstring p4RuntimeEntriesFile = nullptr;

    // Write P4Runtime control plane API description to the specified files.
    cstring p4RuntimeFiles = nullptr;

    // Write static table entries as a P4Runtime WriteRequest message to the specified files.
    cstring p4RuntimeEntriesFiles = nullptr;

    // Choose format for P4Runtime API description.
    P4::P4RuntimeFormat p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;

    // Target
    cstring target = nullptr;

    // Architecture
    cstring arch = nullptr;

    // substrings matched agains pass names
    std::vector<cstring> top4;

    // if this flag is true, compile program in non-debug mode
    bool ndebug = false;

    // Expect that the only remaining argument is the input file.
    void setInputFile();

    // Returns the output of the preprocessor.
    FILE* preprocess();
    // Closes the input stream returned by preprocess.
    void closeInput(FILE* input) const;

    // True if we are compiling a P4 v1.0 or v1.1 program
    bool isv1() const;
    // Get a debug hook function suitable for insertion
    // in the pass managers that are executed.
    DebugHook getDebugHook() const;

    virtual bool enable_intrinsic_metadata_fix();
};


/// A compilation context which exposes compiler options.
class P4CContext : public BaseCompileContext {
 public:
    /// @return the current compilation context, which must inherit from
    /// P4CContext.
    static P4CContext& get();

    P4CContext() {}

    /// @return the compiler options for this compilation context.
    virtual CompilerOptions& options() = 0;

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
    DiagnosticAction
    getDiagnosticAction(cstring diagnostic, DiagnosticAction defaultAction) final {
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
};

/// A utility template which can be used to easily make subclasses of P4CContext
/// which expose a particular subclass of CompilerOptions. This is provided as a
/// convenience since this is all many backends need.
template <typename OptionsType>
class P4CContextWithOptions final : public P4CContext {
 public:
    /// @return the current compilation context, which must be of type
    /// P4CContextWithOptions<OptionsType>.
    static P4CContextWithOptions& get() {
        return CompileContextStack::top<P4CContextWithOptions>();
    }

    /// @return the compiler options for this compilation context.
    OptionsType& options() override { return optionsInstance; }

 private:
    /// Compiler options for this compilation context.
    OptionsType optionsInstance;
};

#endif /* FRONTENDS_COMMON_OPTIONS_H_ */
