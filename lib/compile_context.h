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

#ifndef P4C_LIB_COMPILE_CONTEXT_H_
#define P4C_LIB_COMPILE_CONTEXT_H_

#include <typeinfo>
#include <vector>

#include "lib/cstring.h"
#include "lib/error_reporter.h"

/// An interface for objects which represent compiler settings and state for a
/// translation unit. The compilation context might include things like compiler
/// options which apply to the translation unit or errors and warnings generated
/// by it.
class ICompileContext {
 protected:
    virtual ~ICompileContext() = 0;
};

/// A stack of active compilation contexts. Only the top context is accessible.
/// Compilation contexts can be nested to allow composing programs without
/// intermingling their stack.
struct CompileContextStack final {
    /// @return the current compilation context (i.e., the top of the
    /// compilation context stack), cast to the requested type. If the current
    /// compilation context is of the wrong type, or the stack is empty, an
    /// assertion fires.
    template <typename CompileContextType>
    static CompileContextType& top() {
        auto& stack = getStack();
        if (stack.empty()) reportNoContext();
        auto* current = dynamic_cast<CompileContextType*>(stack.back());
        if (!current) reportContextMismatch(typeid(CompileContextType).name());
        return *current;
    }

 private:
    friend struct AutoCompileContext;

    using StackType = std::vector<ICompileContext*>;

    /// Error reporting helpers.
    static void reportNoContext();
    static void reportContextMismatch(const char* desiredContextType);

    /// Stack manipulation functions.
    static void push(ICompileContext* context);
    static void pop();
    static StackType& getStack();

    CompileContextStack() = delete;
};

/// A RAII helper which pushes a compilation context onto the stack when it's
/// created and pops it off when it's destroyed. To ensure the compilation stack
/// is always nested correctly, this is the only interface for pushing or popping
/// compilation contexts.
struct AutoCompileContext {
    explicit AutoCompileContext(ICompileContext* context);
    ~AutoCompileContext();
};

/// A base compilation context which provides members needed by code in
/// `libp4ctoolkit`. Compilation context types should normally inherit from
/// BaseCompileContext.
class BaseCompileContext : public ICompileContext {
 protected:
    BaseCompileContext();
    BaseCompileContext(const BaseCompileContext& other);

 public:
    /// @return the current compilation context, which must inherit from
    /// BaseCompileContext.
    static BaseCompileContext& get();

    /// @return the error reporter for this compilation context.
    ErrorReporter& errorReporter();

    /// @return the default diagnostic action for calls to `::warning()`.
    virtual DiagnosticAction getDefaultWarningDiagnosticAction();

    /// @return the default diagnostic action for calls to `::error()`.
    virtual DiagnosticAction getDefaultErrorDiagnosticAction();

    /// @return the diagnostic action to use for @diagnosticName, or
    /// @defaultAction if no diagnostic action was found.
    virtual DiagnosticAction
    getDiagnosticAction(cstring diagnostic, DiagnosticAction defaultAction);

 private:
    /// Error and warning tracking facilities for this compilation context.
    ErrorReporter errorReporterInstance;
};

#endif /* P4C_LIB_COMPILE_CONTEXT_H_ */
