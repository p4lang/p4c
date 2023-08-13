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

#include <functional>
#include <ostream>

#include "backends/bmv2/common/midend.h"
#include "backends/bmv2/psa_switch/options.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/parser_options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"
#include "lib/source_file.h"
#include "midend/actionSynthesis.h"
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateSwitch.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/fillEnumMap.h"
#include "midend/flattenHeaders.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/nestedStructs.h"
#include "midend/orderArguments.h"
#include "midend/predication.h"
#include "midend/removeAssertAssume.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"

namespace BMV2 {

/**
This class implements a policy suitable for the ConvertEnums pass.
The policy is: convert all enums that are not part of the psa.
Use 32-bit values for all enums.
Also convert PSA_PacketPath_t to bit<32>
*/
class PsaEnumOn32Bits : public P4::ChooseEnumRepresentation {
    cstring filename;

    bool convert(const IR::Type_Enum *type) const override {
        if (type->name == "PSA_PacketPath_t") return true;
        if (type->name == "PSA_MeterColor_t") return true;
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith(filename))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override { return 32; }

 public:
    explicit PsaEnumOn32Bits(cstring filename) : filename(filename) {}
};

PsaSwitchMidEnd::PsaSwitchMidEnd(CompilerOptions &options, std::ostream *outStream)
    : MidEnd(options) {
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new PsaEnumOn32Bits("psa.p4"));
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    std::function<bool(const Context *, const IR::Expression *)> policy =
        [=](const Context *, const IR::Expression *e) -> bool {
        auto mce = e->to<IR::MethodCallExpression>();
        if (mce == nullptr) return true;
        auto mi = P4::MethodInstance::resolve(mce, &refMap, &typeMap);
        auto em = mi->to<P4::ExternMethod>();
        if (em == nullptr) return true;
        if (em->originalExternType->name.name == "Register" || em->method->name.name == "read")
            return false;
        if (em->originalExternType->name.name == "Meter" && em->method->name.name == "execute")
            return false;
        return true;
    };
    if (BMV2::PsaSwitchContext::get().options().loadIRFromJson == false) {
        addPasses({
            options.ndebug ? new P4::RemoveAssertAssume(&refMap, &typeMap) : nullptr,
            new P4::RemoveMiss(&refMap, &typeMap),
            new P4::EliminateNewtype(&refMap, &typeMap),
            new P4::EliminateInvalidHeaders(&refMap, &typeMap),
            new P4::EliminateSerEnums(&refMap, &typeMap),
            convertEnums,
            [this, convertEnums]() { enumMap = convertEnums->getEnumMapping(); },
            new P4::OrderArguments(&refMap, &typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::SimplifyKey(
                &refMap, &typeMap,
                new P4::OrPolicy(new P4::IsValid(&refMap, &typeMap), new P4::IsMask())),
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::StrengthReduction(&refMap, &typeMap),
            new P4::SimplifySelectCases(&refMap, &typeMap, true),  // require constant keysets
            new P4::ExpandLookahead(&refMap, &typeMap),
            new P4::ExpandEmit(&refMap, &typeMap),
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
            new P4::ReplaceSelectRange(&refMap, &typeMap),
            new P4::Predication(&refMap),
            new P4::MoveDeclarations(),  // more may have been introduced
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::LocalCopyPropagation(&refMap, &typeMap, nullptr, policy),
            new PassRepeated({new P4::ConstantFolding(&refMap, &typeMap),
                              new P4::StrengthReduction(&refMap, &typeMap)}),
            new P4::MoveDeclarations(),
            new P4::ValidateTableProperties({"psa_implementation", "psa_direct_counter",
                                             "psa_direct_meter", "psa_idle_timeout", "size"}),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::CompileTimeOperations(),
            new P4::TableHit(&refMap, &typeMap),
            new P4::EliminateSwitch(&refMap, &typeMap),
            new P4::MoveActionsToTables(&refMap, &typeMap),
            new P4::RemoveLeftSlices(&refMap, &typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::MidEndLast(),
            evaluator,
            [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
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
        auto fillEnumMap = new P4::FillEnumMap(new PsaEnumOn32Bits("psa.p4"), &typeMap);
        addPasses({
            new P4::ResolveReferences(&refMap),
            new P4::TypeChecking(&refMap, &typeMap),
            fillEnumMap,
            [this, fillEnumMap]() { enumMap = fillEnumMap->repr; },
            evaluator,
            [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
        });
    }
}

}  // namespace BMV2
