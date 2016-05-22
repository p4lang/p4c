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

#include "midend.h"
#include "midend/actionsInlining.h"
#include "midend/inlining.h"
#include "midend/moveDeclarations.h"
#include "midend/uniqueNames.h"
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/actionSynthesis.h"
#include "midend/localizeActions.h"
#include "midend/removeParameters.h"
#include "midend/local_copyprop.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"

namespace EBPF {

const IR::ToplevelBlock* MidEnd::run(EbpfOptions& options, const IR::P4Program* program) {
    if (program == nullptr)
        return nullptr;

    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);

    PassManager simplify = {
        // Proper semantics for uninitialzed local variables in parser states:
        // headers must be invalidated
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::ResetHeaders(&typeMap),
        // Give each local declaration a unique internal name
        new P4::UniqueNames(&refMap, isv1),
        // Move all local declarations to the beginning
        new P4::MoveDeclarations(),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveReturns(&refMap),  // necessary for inlining
        // Move some constructor calls into temporaries
        new P4::MoveConstructors(isv1),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveUnusedDeclarations(&refMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        evaluator,
    };

    simplify.setName("Simplify");
    simplify.addDebugHooks(hooks);
    program = program->apply(simplify);
    if (::errorCount() > 0)
        return nullptr;
    auto toplevel = evaluator->getToplevelBlock();
    if (toplevel->getMain() == nullptr)
        // nothing further to do
        return nullptr;

    P4::InlineWorkList toInline;
    P4::ActionsInlineList actionsToInline;

    PassManager midEnd = {
        // Perform inlining for controls and parsers (parsers not yet implemented)
        new P4::DiscoverInlining(&toInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&toInline, new P4::GeneralInliner(), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        // Perform inlining for actions calling other actions
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
        new P4::InlineActionsDriver(&actionsToInline, new P4::ActionsInliner(), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        // TODO: simplify statements and expressions.
        // This is required for the correctness of some of the following passes.

        // Clone an action for each use, so we can specialize the action
        // per user (e.g., for each table or direct invocation).
        new P4::LocalizeAllActions(&refMap, isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        // Table and action parameters also get unique names
        new P4::UniqueParameters(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::RemoveTableParameters(&refMap, &typeMap),
        // Exit statements are transformed into control-flow
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::RemoveExits(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::TypeChecking(&refMap, &typeMap, isv1, true),
        new P4::LocalCopyPropagation(),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        evaluator,
    };
    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::errorCount() > 0)
        return nullptr;

    return evaluator->getToplevelBlock();
}

}  // namespace EBPF
