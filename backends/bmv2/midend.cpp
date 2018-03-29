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
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend/actionSynthesis.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateTuples.h"
#include "midend/local_copyprop.h"
#include "midend/nestedStructs.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeParameters.h"
#include "midend/removeUnusedParameters.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/removeSelectBooleans.h"
#include "midend/validateProperties.h"
#include "midend/compileTimeOps.h"
#include "midend/predication.h"
#include "midend/expandLookahead.h"
#include "midend/expandEmit.h"
#include "midend/tableHit.h"
#include "midend/midEndLast.h"

namespace BMV2 {

/**
This class implements a policy suitable for the ConvertEnums pass.
The policy is: convert all enums that are not part of the v1model.
Use 32-bit values for all enums.
*/
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum* type) const override {
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith(P4V1::V1Model::instance.file.name))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override
    { return 32; }
};

MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.isv1();
    setName("MidEnd");
    refMap.setIsV1(isv1);  // must be done BEFORE creating passes
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    addPasses({
        new P4::RemoveActionParameters(&refMap, &typeMap),
        convertEnums,
        new VisitFunctor([this, convertEnums]() { enumMap = convertEnums->getEnumMapping(); }),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::OrPolicy(
                                new P4::IsValid(&refMap, &typeMap),
                                new P4::IsMask())),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::SimplifySelectCases(&refMap, &typeMap, true),  // require constant keysets
        new P4::ExpandLookahead(&refMap, &typeMap),
        new P4::ExpandEmit(&refMap, &typeMap),
        new P4::SimplifyParsers(&refMap),
        new P4::StrengthReduction(),
        new P4::EliminateTuples(&refMap, &typeMap),
        new P4::SimplifyComparisons(&refMap, &typeMap),
        new P4::CopyStructures(&refMap, &typeMap),
        new P4::NestedStructs(&refMap, &typeMap),
        new P4::SimplifySelectList(&refMap, &typeMap),
        new P4::RemoveSelectBooleans(&refMap, &typeMap),
        new P4::Predication(&refMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::MoveDeclarations(),
        new P4::ValidateTableProperties({ "implementation", "size", "counters",
                                          "meters", "support_timeout" }),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::CompileTimeOperations(),
        new P4::TableHit(&refMap, &typeMap),
        new P4::RemoveLeftSlices(&refMap, &typeMap),

        // p4c-bm removed unused action parameters. To produce a compatible
        // control plane API, we remove them as well for P4-14 programs.
        isv1 ? new P4::RemoveUnusedActionParameters(&refMap) : nullptr,

        new P4::TypeChecking(&refMap, &typeMap),
        new P4::MidEndLast(),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
    });
}

}  // namespace BMV2
