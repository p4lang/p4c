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
#include "lower.h"
#include "inlining.h"
#include "midend/actionsInlining.h"
#include "midend/uniqueNames.h"
#include "midend/moveDeclarations.h"
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/localizeActions.h"
#include "midend/removeParameters.h"
#include "midend/actionSynthesis.h"
#include "midend/local_copyprop.h"
#include "midend/removeLeftSlices.h"
#include "midend/convertEnums.h"
#include "midend/simplifyKey.h"
#include "midend/simplifyExpressions.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/fromv1.0/v1model.h"

namespace BMV2 {

void MidEnd::setup_for_P4_14(CompilerOptions&) {
    bool isv1 = true;
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);

    // Inlining is simpler for P4 v1.0/1.1 programs, so we have a
    // specialized code path, which also generates slighly nicer
    // human-readable results.

    addPasses({
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        evaluator,
        new P4::DiscoverInlining(&controlsToInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&controlsToInline, new SimpleControlsInliner(&refMap), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
        new P4::InlineActionsDriver(&actionsToInline, new SimpleActionsInliner(&refMap), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
    });
}

class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum* type) const override {
        if (type->srcInfo.isValid()) {
            unsigned line = type->srcInfo.getStart().getLineNumber();
            auto sfl = Util::InputSources::instance->getSourceLine(line);
            cstring sourceFile = sfl.fileName;
            if (sourceFile.endsWith(P4V1::V1Model::instance.file.name))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override
    { return 32; }
};


void MidEnd::setup_for_P4_16(CompilerOptions& options) {
    // we may come through this path even if the program is actually a P4 v1.0 program
    bool isv1 = options.isv1();
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);

    addPasses({
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::ConvertEnums(new EnumOn32Bits(), &typeMap),
        // Proper semantics for uninitialzed local variables in parser states:
        // headers must be invalidated.  Must recompute all types after ConvertEnums
        new P4::TypeChecking(&refMap, &typeMap, true, isv1),
        new P4::ResetHeaders(&typeMap),
        // Give each local declaration a unique internal name
        new P4::UniqueNames(&refMap, isv1),
        // Move all local declarations to the beginning
        new P4::MoveDeclarations(),
        new P4::MoveInitializers(),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::SimplifyExpressions(&refMap, &typeMap),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveReturns(&refMap),
        // Move some constructor calls into temporaries
        new P4::MoveConstructors(&refMap, isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, true, isv1),
        evaluator,

        new VisitFunctor([evaluator](const IR::Node *root) -> const IR::Node * {
            auto toplevel = evaluator->getToplevelBlock();
            if (toplevel->getMain() == nullptr)
                // nothing further to do
                return nullptr;
            return root; }),

        // Inlining
        new P4::DiscoverInlining(&controlsToInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&controlsToInline, new P4::GeneralInliner(isv1), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
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
        // Clear types after LocalizeAllActions
        new P4::TypeChecking(&refMap, &typeMap, true, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveParameters(&refMap, &typeMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, true, isv1),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::NonLeftValue(&refMap, &typeMap)),
        // Final simplifications
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::LocalCopyPropagation(&typeMap),
        new P4::MoveDeclarations(),
        // Create actions for statements that can't be done in control blocks.
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::SynthesizeActions(&refMap, &typeMap),
        // Move all stand-alone actions to custom tables
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::MoveActionsToTables(&refMap, &typeMap),
    });
}


MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.isv1();

    setName("MidEnd");
    if (isv1)
        // TODO: This path should be eventually deprecated
        setup_for_P4_14(options);
    else
        setup_for_P4_16(options);

    // BMv2-specific passes
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);
    addPasses({
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::RemoveLeftSlices(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new LowerExpressions(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); })
    });
}

}  // namespace BMV2
