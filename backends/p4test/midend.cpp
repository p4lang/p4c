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
#include "midend/removeParameters.h"
#include "midend/moveConstructors.h"
#include "midend/actionSynthesis.h"
#include "midend/localizeActions.h"
#include "midend/local_copyprop.h"
#include "midend/simplifyKey.h"
#include "midend/parserUnroll.h"
#include "midend/specialize.h"
#include "midend/simplifySelect.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/uniqueNames.h"

namespace P4Test {

MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    setName("MidEnd");

    // TODO: def-use analysis and related optimizations
    // TODO: parser loop unrolling
    // TODO: improve copy propagation
    // TODO: simplify actions which are too complex
    // TODO: lower errors to integers
    // TODO: handle bit-slices as out arguments
    // TODO: remove control-flow from parsers
    addPasses({
        new P4::RemoveReturns(&refMap),
        new P4::MoveConstructors(&refMap),
        new P4::RemoveAllUnusedDeclarations(&refMap),
        new P4::ClearTypeMap(&typeMap),
        evaluator,
        new VisitFunctor([evaluator](const IR::Node *root) -> const IR::Node * {
            auto toplevel = evaluator->getToplevelBlock();
            if (toplevel->getMain() == nullptr)
                // nothing further to do
                return nullptr;
            return root; }),
        new P4::Inline(&refMap, &typeMap, evaluator),
        new P4::InlineActions(&refMap, &typeMap),
        new P4::SpecializeAll(&refMap, &typeMap),
        // Parser loop unrolling: TODO
        // new P4::ParsersUnroll(true, &refMap, &typeMap),
        new P4::LocalizeAllActions(&refMap),
        new P4::UniqueNames(&refMap),
        new P4::UniqueParameters(&refMap),
        new P4::ClearTypeMap(&typeMap),  // table types have changed
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveTableParameters(&refMap, &typeMap),
        new P4::RemoveActionParameters(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::NonLeftValue(&refMap, &typeMap)),
        new P4::RemoveExits(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifySelect(&refMap, &typeMap, false),  // non-constant keysets
        new P4::SimplifyParsers(&refMap),
        new P4::StrengthReduction(),
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::SynthesizeActions(&refMap, &typeMap),
        new P4::MoveActionsToTables(&refMap, &typeMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); })
    });
}

}  // namespace P4Test
