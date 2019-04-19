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

#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend.h"
#include "midend/actionSynthesis.h"
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/copyStructures.h"
#include "midend/eliminateTuples.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/flattenHeaders.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/nestedStructs.h"
#include "midend/noMatch.h"
#include "midend/parserUnroll.h"
#include "midend/predication.h"
#include "midend/removeExits.h"
#include "midend/removeParameters.h"
#include "midend/removeSelectBooleans.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/tableHit.h"

namespace P4Test {

class SkipControls : public P4::ActionSynthesisPolicy {
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const Visitor::Context *, const IR::P4Control* control) override {
        if (skip->find(control->name) != skip->end())
            return false;
        return true;
    }
};

MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    setName("MidEnd");

    auto v1controls = new std::set<cstring>();

    // TODO: parser loop unrolling
    // TODO: lower errors to integers
    // TODO: handle bit-slices as out arguments
    addPasses({
        new P4::EliminateNewtype(&refMap, &typeMap),
        new P4::EliminateSerEnums(&refMap, &typeMap),
        new P4::RemoveActionParameters(&refMap, &typeMap),
        new P4::SimplifyKey(&refMap, &typeMap,
                            new P4::OrPolicy(
                                new P4::IsValid(&refMap, &typeMap),
                                new P4::IsLikeLeftValue())),
        new P4::RemoveExits(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifySelectCases(&refMap, &typeMap, false),  // non-constant keysets
        new P4::ExpandLookahead(&refMap, &typeMap),
        new P4::ExpandEmit(&refMap, &typeMap),
        new P4::HandleNoMatch(&refMap),
        new P4::SimplifyParsers(&refMap),
        new P4::StrengthReduction(&refMap, &typeMap),
        new P4::EliminateTuples(&refMap, &typeMap),
        new P4::SimplifyComparisons(&refMap, &typeMap),
        new P4::CopyStructures(&refMap, &typeMap),
        new P4::NestedStructs(&refMap, &typeMap),
        new P4::SimplifySelectList(&refMap, &typeMap),
        new P4::RemoveSelectBooleans(&refMap, &typeMap),
        new P4::FlattenHeaders(&refMap, &typeMap),
        new P4::FlattenInterfaceStructs(&refMap, &typeMap),
        new P4::Predication(&refMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::CompileTimeOperations(),
        new P4::TableHit(&refMap, &typeMap),
        evaluator,
        new VisitFunctor([v1controls, evaluator](const IR::Node *root) -> const IR::Node * {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            if (main == nullptr)
                // nothing further to do
                return nullptr;
            // Special handling when compiling for v1model.p4
            if (main->type->name == P4V1::V1Model::instance.sw.name) {
                if (main->getConstructorParameters()->size() != 6)
                    return root;
                auto verify = main->getParameterValue(P4V1::V1Model::instance.sw.verify.name);
                auto update = main->getParameterValue(
                    P4V1::V1Model::instance.sw.compute.name);
                auto deparser = main->getParameterValue(P4V1::V1Model::instance.sw.deparser.name);
                if (verify == nullptr || update == nullptr || deparser == nullptr ||
                    !verify->is<IR::ControlBlock>() || !update->is<IR::ControlBlock>() ||
                    !deparser->is<IR::ControlBlock>()) {
                    return root;
                }
                v1controls->emplace(verify->to<IR::ControlBlock>()->container->name);
                v1controls->emplace(update->to<IR::ControlBlock>()->container->name);
                v1controls->emplace(deparser->to<IR::ControlBlock>()->container->name);
            }
            return root; }),
        new P4::SynthesizeActions(&refMap, &typeMap, new SkipControls(v1controls)),
        new P4::MoveActionsToTables(&refMap, &typeMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
        new P4::MidEndLast()
    });
}

}  // namespace P4Test
