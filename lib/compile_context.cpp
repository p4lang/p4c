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

#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/exceptions.h"

ICompileContext::~ICompileContext() { }

/* static */ void CompileContextStack::push(ICompileContext* context) {
    BUG_CHECK(context != nullptr, "Pushing a null CompileContext");
    getStack().push_back(context);
}

/* static */ void CompileContextStack::pop() {
    BUG_CHECK(!getStack().empty(),
              "Popping an empty CompileContextStack");
    getStack().pop_back();
}

/* static */ void CompileContextStack::reportNoContext() {
    BUG("CompileContextStack has an empty CompileContext stack");
}

/* static */ void
CompileContextStack::reportContextMismatch(const char* desiredContextType) {
    BUG_CHECK(!getStack().empty(),
              "Reporting a CompileContext type mismatch, but the "
              "CompileContext stack is empty");
    auto* topContext = getStack().back();
    BUG("The top of the CompileContextStack has type %1% but we attempted to "
        "find a context of type %2%",
        typeid(*topContext).name(), desiredContextType);
}

/* static */ CompileContextStack::StackType& CompileContextStack::getStack() {
    static StackType stack;
    return stack;
}

AutoCompileContext::AutoCompileContext(ICompileContext* context) {
    CompileContextStack::push(context);
}

AutoCompileContext::~AutoCompileContext() {
    CompileContextStack::pop();
}

BaseCompileContext::BaseCompileContext() { }

BaseCompileContext::BaseCompileContext(const BaseCompileContext& other)
    : errorReporterInstance(other.errorReporterInstance) { }

/* static */ BaseCompileContext& BaseCompileContext::get() {
    return CompileContextStack::top<BaseCompileContext>();
}

ErrorReporter& BaseCompileContext::errorReporter() {
    return errorReporterInstance;
}

DiagnosticAction BaseCompileContext::getDefaultWarningDiagnosticAction() {
    return DiagnosticAction::Warn;
}

DiagnosticAction BaseCompileContext::getDefaultErrorDiagnosticAction() {
    return DiagnosticAction::Error;
}

DiagnosticAction
BaseCompileContext::getDiagnosticAction(cstring /* diagnostic */,
                                        DiagnosticAction defaultAction) {
    return defaultAction;
}
