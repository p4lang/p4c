// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/compile_context.h"

#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4 {

ICompileContext::~ICompileContext() {}

/* static */ void CompileContextStack::push(ICompileContext *context) {
    BUG_CHECK(context != nullptr, "Pushing a null CompileContext");
    getStack().push_back(context);
}

/* static */ void CompileContextStack::pop() {
    BUG_CHECK(!getStack().empty(), "Popping an empty CompileContextStack");
    getStack().pop_back();
}

/* static */ void CompileContextStack::reportNoContext() {
    BUG("CompileContextStack has an empty CompileContext stack");
}

/* static */ void CompileContextStack::reportContextMismatch(const char *desiredContextType) {
    BUG_CHECK(!getStack().empty(),
              "Reporting a CompileContext type mismatch, but the "
              "CompileContext stack is empty");
    auto *topContext = getStack().back();
    BUG("The top of the CompileContextStack has type %1% but we attempted to "
        "find a context of type %2%",
        typeid(*topContext).name(), desiredContextType);
}

/* static */ CompileContextStack::StackType &CompileContextStack::getStack() {
    static StackType stack;
    return stack;
}

AutoCompileContext::AutoCompileContext(ICompileContext *context) {
    CompileContextStack::push(context);
}

AutoCompileContext::~AutoCompileContext() { CompileContextStack::pop(); }

/* static */ BaseCompileContext &BaseCompileContext::get() {
    return CompileContextStack::top<BaseCompileContext>();
}

ErrorReporter &BaseCompileContext::errorReporter() { return errorReporterInstance; }

DiagnosticAction BaseCompileContext::getDefaultInfoDiagnosticAction() {
    return DiagnosticAction::Info;
}

DiagnosticAction BaseCompileContext::getDefaultWarningDiagnosticAction() {
    return DiagnosticAction::Warn;
}

DiagnosticAction BaseCompileContext::getDefaultErrorDiagnosticAction() {
    return DiagnosticAction::Error;
}

}  // namespace P4
