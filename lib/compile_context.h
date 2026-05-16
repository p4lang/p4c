/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_COMPILE_CONTEXT_H_
#define LIB_COMPILE_CONTEXT_H_

#include <typeinfo>
#include <vector>

#include "lib/cstring.h"
#include "lib/error_reporter.h"

namespace P4 {

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
    CompileContextStack() = delete;

    /// @return the current compilation context (i.e., the top of the
    /// compilation context stack), cast to the requested type. If the current
    /// compilation context is of the wrong type, or the stack is empty, an
    /// assertion fires.
    template <typename CompileContextType>
    static CompileContextType &top() {
        auto &stack = getStack();
        if (stack.empty()) reportNoContext();
        auto *current = dynamic_cast<CompileContextType *>(stack.back());
        if (!current) reportContextMismatch(typeid(CompileContextType).name());
        return *current;
    }

    static bool isEmpty() { return getStack().empty(); }

 private:
    friend struct AutoCompileContext;

    using StackType = std::vector<ICompileContext *>;

    /// Error reporting helpers.
    static void reportNoContext();
    static void reportContextMismatch(const char *desiredContextType);

    /// Stack manipulation functions.
    static void push(ICompileContext *context);
    static void pop();
    static StackType &getStack();
};

/// A RAII helper which pushes a compilation context onto the stack when it's
/// created and pops it off when it's destroyed. To ensure the compilation stack
/// is always nested correctly, this is the only interface for pushing or popping
/// compilation contexts.
struct AutoCompileContext {
    explicit AutoCompileContext(ICompileContext *context);
    ~AutoCompileContext();
};

/// A base compilation context which provides members needed by code in
/// `libp4ctoolkit`. Compilation context types should normally inherit from
/// BaseCompileContext.
class BaseCompileContext : public ICompileContext {
 protected:
    BaseCompileContext() = default;
    BaseCompileContext(const BaseCompileContext &other) = default;
    BaseCompileContext &operator=(const BaseCompileContext &other) = default;

 public:
    /// @return the current compilation context, which must inherit from
    /// BaseCompileContext.
    static BaseCompileContext &get();

    /// @return the error reporter for this compilation context.
    virtual ErrorReporter &errorReporter();

    /// @return the default diagnostic action for calls to `::P4::info()`.
    virtual DiagnosticAction getDefaultInfoDiagnosticAction();

    /// @return the default diagnostic action for calls to `::P4::warning()`.
    virtual DiagnosticAction getDefaultWarningDiagnosticAction();

    /// @return the default diagnostic action for calls to `::P4::error()`.
    virtual DiagnosticAction getDefaultErrorDiagnosticAction();

 private:
    /// Error and warning tracking facilities for this compilation context.
    ErrorReporter errorReporterInstance;
};

}  // namespace P4

#endif /* LIB_COMPILE_CONTEXT_H_ */
