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
#include "midend/actionsInlining.h"
#include "midend/actionSynthesis.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateTuples.h"
#include "midend/isolateMethodCalls.h"
#include "midend/local_copyprop.h"
#include "midend/localizeActions.h"
#include "midend/moveConstructors.h"
#include "midend/nestedStructs.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeParameters.h"
#include "midend/removeReturns.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/removeSelectBooleans.h"
#include "midend/validateProperties.h"
#include "midend/compileTimeOps.h"
#include "midend/predication.h"
#include "midend/expandLookahead.h"
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

/**
This class implements a policy suitable for the SynthesizeActions pass.
The policy is: do not synthesize actions for the controls whose names
are in the specified set.
For example, we expect that the code in the deparser will not use any
tables or actions.
*/
class SkipControls : public P4::ActionSynthesisPolicy {
    // set of controls where actions are not synthesized
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const IR::P4Control* control) const {
        if (skip->find(control->name) != skip->end())
            return false;
        return true;
    }
};

class GenerateSkipControls : public Inspector {
    std::set<cstring>* skip;
    BlockTypeMap*      map;
 public:
    explicit GenerateSkipControls(BlockTypeMap* map, std::set<cstring>* skip) :
        skip(skip), map(map) { CHECK_NULL(skip); }
    bool preorder(const IR::ControlBlock* block) {
        auto bt = map->find(block);
        if (bt != map->end()) {
            if (!bt->second->getAnnotation("pipeline")) {
                LOG1("found pipeline");
                skip->insert(block->container->name);
            }
        }
        return false;
    }
    bool preorder(const IR::PackageBlock* block) {
        for (auto it : block->constantValue) {
            if (it.second->is<IR::ControlBlock>()) {
                visit(it.second->getNode());
            }
        }
        return false;
    }
};

MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.isv1();
    setName("MidEnd");
    refMap.setIsV1(isv1);  // must be done BEFORE creating passes
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    auto skipv1controls = new std::set<cstring>();  // in these controls we don't synthesize actions
    auto mapBlockType = new CopyAnnotations(&refMap, &blockTypeMap);
    auto generateSkipControls = new GenerateSkipControls(&blockTypeMap, skipv1controls);

    addPasses({
        convertEnums,
        new VisitFunctor([this, convertEnums]() { enumMap = convertEnums->getEnumMapping(); }),
        new P4::IsolateMethodCalls(&refMap, &typeMap),
        new P4::RemoveReturns(&refMap),
        new P4::MoveConstructors(&refMap),
        new P4::RemoveAllUnusedDeclarations(&refMap),
        new P4::ClearTypeMap(&typeMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
        // build the map from user block to architecture type
        new VisitFunctor([this, mapBlockType]() { toplevel->getMain()->apply(*mapBlockType); }),
        new VisitFunctor([this, generateSkipControls]() { toplevel->getMain()->apply(*generateSkipControls); }),
        new VisitFunctor([this]() { for(auto a : blockTypeMap) LOG1("skip" << a.first << " " << a.second); }),
        new P4::Inline(&refMap, &typeMap, evaluator),
        new P4::InlineActions(&refMap, &typeMap),
        new P4::LocalizeAllActions(&refMap),
        new P4::UniqueNames(&refMap),  // needed again after inlining
        new P4::UniqueParameters(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveActionParameters(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::NonMaskLeftValue(&refMap, &typeMap)),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::SimplifySelectCases(&refMap, &typeMap, true),  // require constant keysets
        new P4::ExpandLookahead(&refMap, &typeMap),
        new P4::SimplifyParsers(&refMap),
        new P4::StrengthReduction(),
        new P4::EliminateTuples(&refMap, &typeMap),
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
                                          "meters", "size", "support_timeout" }),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::CompileTimeOperations(),
        new P4::TableHit(&refMap, &typeMap),
        // TODO(hanw): skip synthesizing actions in verify, update and deparser.
        new P4::SynthesizeActions(&refMap, &typeMap, new SkipControls(skipv1controls)),
        new P4::MoveActionsToTables(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveLeftSlices(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        new LowerExpressions(&typeMap),
        new P4::ConstantFolding(&refMap, &typeMap, false),
        new P4::TypeChecking(&refMap, &typeMap),
        // TODO(hanw): re-enable these two passes
        //new RemoveComplexExpressions(&refMap, &typeMap,
        //                             &ingressControlBlockName, &egressControlBlockName),
        //new FixupChecksum(&updateControlBlockName),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveAllUnusedDeclarations(&refMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
        new P4::MidEndLast()
    });
}

}  // namespace BMV2
