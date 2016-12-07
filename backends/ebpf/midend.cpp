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
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/actionSynthesis.h"
#include "midend/localizeActions.h"
#include "midend/removeParameters.h"
#include "midend/local_copyprop.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelect.h"
#include "midend/validateProperties.h"
#include "midend/eliminateTuples.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/simplifyParsers.h"

namespace EBPF {

const IR::ToplevelBlock* MidEnd::run(EbpfOptions& options, const IR::P4Program* program) {
    if (program == nullptr)
        return nullptr;

    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager simplify = {
        new P4::RemoveReturns(&refMap),
        new P4::MoveConstructors(&refMap),
        new P4::RemoveAllUnusedDeclarations(&refMap),
        new P4::ClearTypeMap(&typeMap),
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
        new P4::Inline(&refMap, &typeMap, evaluator),
        new P4::InlineActions(&refMap, &typeMap),
        new P4::LocalizeAllActions(&refMap),
        new P4::UniqueNames(&refMap),
        new P4::UniqueParameters(&refMap, &typeMap),
        new P4::ClearTypeMap(&typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveTableParameters(&refMap, &typeMap),
        new P4::RemoveActionParameters(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::NonLeftValue(&refMap, &typeMap)),
        new P4::RemoveExits(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifySelect(&refMap, &typeMap, false),  // accept non-constant keysets
        new P4::SimplifyParsers(&refMap),
        new P4::StrengthReduction(),
        new P4::EliminateTuples(&refMap, &typeMap),
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::ValidateTableProperties({"implementation"}),
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
