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
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/localizeActions.h"
#include "midend/removeParameters.h"
#include "midend/actionSynthesis.h"
#include "midend/local_copyprop.h"
#include "midend/removeLeftSlices.h"
#include "midend/convertEnums.h"
#include "midend/simplifyKey.h"
#include "midend/parserControlFlow.h"
#include "midend/simplifySelect.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/fromv1.0/v1model.h"

namespace BMV2 {

void MidEnd::setup_for_P4_14(CompilerOptions&) {
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    // Inlining is simpler for P4 v1.0/1.1 programs, so we have a
    // specialized code path, which also generates slighly nicer
    // human-readable results.
    addPasses({
        evaluator,
        new P4::DiscoverInlining(&controlsToInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&controlsToInline, new SimpleControlsInliner(&refMap)),
        new P4::RemoveAllUnusedDeclarations(&refMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
        new P4::InlineActionsDriver(&actionsToInline, new SimpleActionsInliner(&refMap)),
        new P4::RemoveAllUnusedDeclarations(&refMap),
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


void MidEnd::setup_for_P4_16(CompilerOptions&) {
    // we may come through this path even if the program is actually a P4 v1.0 program
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    addPasses({
        new P4::RemoveParserControlFlow(&refMap, &typeMap),
        new P4::ConvertEnums(&refMap, &typeMap,
                             new EnumOn32Bits()),
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
        new P4::LocalizeAllActions(&refMap),
        new P4::UniqueNames(&refMap),
        new P4::UniqueParameters(&refMap),
        new P4::ClearTypeMap(&typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveTableParameters(&refMap, &typeMap),
        new P4::RemoveActionParameters(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::NonLeftValue(&refMap, &typeMap)),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::SimplifySelect(&refMap, &typeMap, true),  // require constant keysets
        new P4::SimplifyParsers(&refMap),
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new P4::MoveDeclarations(),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::SynthesizeActions(&refMap, &typeMap),
        new P4::MoveActionsToTables(&refMap, &typeMap),
    });
}


MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.isv1();
    setName("MidEnd");
    refMap.setIsV1(isv1);  // must be done BEFORE creating passes
    if (isv1)
        // TODO: This path should be eventually deprecated
        setup_for_P4_14(options);
    else
        setup_for_P4_16(options);

    // BMv2-specific passes
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    addPasses({
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::RemoveLeftSlices(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new LowerExpressions(&typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); })
    });
}

}  // namespace BMV2
