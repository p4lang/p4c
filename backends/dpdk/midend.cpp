/*
Copyright 2020 Intel Corp.

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
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/fillEnumMap.h"
#include "midend/flattenHeaders.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/nestedStructs.h"
#include "midend/noMatch.h"
#include "midend/orderArguments.h"
#include "midend/predication.h"
#include "midend/parserUnroll.h"
#include "midend/removeAssertAssume.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeExits.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/removeUnusedParameters.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"
#include "options.h"

namespace DPDK {

/**
This class implements a policy suitable for the ConvertEnums pass.
The policy is: convert all enums to bit<32>
*/
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum *) const override {
        return true;
    }
    unsigned enumSize(unsigned) const override { return 32; }
 public:
    EnumOn32Bits() {}
};

PsaSwitchMidEnd::PsaSwitchMidEnd(CompilerOptions &options,
                                 std::ostream *outStream)
    : MidEnd(options) {
    auto convertEnums =
        new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    std::function<bool(const Context *, const IR::Expression *)> policy =
        [=](const Context *ctx, const IR::Expression *e) -> bool {
        if (auto mce = findContext<IR::MethodCallExpression>(ctx)) {
            auto mi = P4::MethodInstance::resolve(mce, &refMap, &typeMap);
            if (auto em = mi->to<P4::ExternMethod>()) {
                cstring externType = em->originalExternType->getName().name;
                cstring externMethod = em->method->getName().name;

                std::vector<std::pair<cstring, cstring>> doNotCopyPropList = {
                    {"Checksum", "update"},
                    {"Hash", "get_hash"},
                    {"InternetChecksum", "add"},
                    {"InternetChecksum", "subtract"},
                    {"InternetChecksum", "set_state"},
                    {"Register", "read"},
                    {"Register", "write"},
                    {"Counter", "count"},
                    {"Meter", "execute"},
                    {"Digest", "pack"},
                };
                for (auto f : doNotCopyPropList) {
                    if (externType == f.first && externMethod == f.second) {
                        return false; } }
            } else if (auto ef = mi->to<P4::ExternFunction>()) {
                cstring externFuncName = ef->method->getName().name;
                std::vector<cstring> doNotCopyPropList = {
                    "verify",
                };
                for (auto f : doNotCopyPropList) {
                    if (externFuncName == f)
                        return false;
                }
            }
        }
        return true;
    };
    if (DPDK::PsaSwitchContext::get().options().loadIRFromJson == false) {
        addPasses({
            options.ndebug ? new P4::RemoveAssertAssume(&refMap, &typeMap)
                           : nullptr,
            new P4::RemoveMiss(&refMap, &typeMap),
            new P4::EliminateNewtype(&refMap, &typeMap),
            new P4::EliminateSerEnums(&refMap, &typeMap),
            convertEnums,
            new VisitFunctor([this, convertEnums]() {
                enumMap = convertEnums->getEnumMapping();
            }),
            new P4::OrderArguments(&refMap, &typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::SimplifyKey(
                &refMap, &typeMap,
                new P4::OrPolicy(new P4::IsValid(&refMap, &typeMap),
                                 new P4::IsMask())),
            new P4::RemoveExits(&refMap, &typeMap),
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::StrengthReduction(&refMap, &typeMap),
            new P4::SimplifySelectCases(&refMap, &typeMap, true),
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
            new P4::ParsersUnroll(true, &refMap, &typeMap),
            new P4::ReplaceSelectRange(&refMap, &typeMap),
            // DPDK architecture does not currently support predicated instructions
            // new P4::Predication(&refMap),
            new P4::MoveDeclarations(),  // more may have been introduced
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::LocalCopyPropagation(&refMap, &typeMap, nullptr, policy),
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::MoveDeclarations(),
            new P4::ValidateTableProperties(
                {"psa_implementation", "psa_direct_counter", "psa_direct_meter",
                 "psa_idle_timeout", "size"}),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::CompileTimeOperations(),
            new P4::TableHit(&refMap, &typeMap),
            new P4::RemoveLeftSlices(&refMap, &typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::MidEndLast(),
            evaluator,
            new VisitFunctor([this, evaluator]() {
                toplevel = evaluator->getToplevelBlock();
            }),
        });
        if (options.listMidendPasses) {
            listPasses(*outStream, "\n");
            *outStream << std::endl;
            return;
        }
        if (options.excludeMidendPasses) {
            removePasses(options.passesToExcludeMidend);
        }
    } else {
        auto fillEnumMap =
            new P4::FillEnumMap(new EnumOn32Bits(), &typeMap);
        addPasses({
            new P4::ResolveReferences(&refMap),
            new P4::TypeChecking(&refMap, &typeMap),
            fillEnumMap,
            new VisitFunctor(
                [this, fillEnumMap]() { enumMap = fillEnumMap->repr; }),
            evaluator,
            new VisitFunctor([this, evaluator]() {
                toplevel = evaluator->getToplevelBlock();
            }),
        });
    }
}

}  // namespace DPDK
