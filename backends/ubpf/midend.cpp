/*
Copyright 2019 Orange

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

#include <ostream>

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/lower.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/source_file.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/noMatch.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/singleArgumentSelect.h"
#include "midend/tableHit.h"

namespace UBPF {

class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum *type) const override {
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith("_model.p4"))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override { return 32; }
};

const IR::ToplevelBlock *MidEnd::run(EbpfOptions &options, const IR::P4Program *program,
                                     std::ostream *outStream) {
    if (program == nullptr && options.listMidendPasses == 0) return nullptr;

    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager midEnd;
    if (options.loadIRFromJson == false) {
        midEnd.addPasses(
            {new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits()),
             new P4::RemoveMiss(&refMap, &typeMap), new P4::ClearTypeMap(&typeMap),
             new P4::EliminateNewtype(&refMap, &typeMap),
             new P4::EliminateInvalidHeaders(&refMap, &typeMap),
             new P4::SimplifyControlFlow(&refMap, &typeMap),
             new P4::SimplifyKey(
                 &refMap, &typeMap,
                 new P4::OrPolicy(new P4::IsValid(&refMap, &typeMap), new P4::IsLikeLeftValue())),
             new P4::ConstantFolding(&refMap, &typeMap),
             // accept non-constant keysets
             new P4::SimplifySelectCases(&refMap, &typeMap, false), new P4::HandleNoMatch(&refMap),
             new P4::SimplifyParsers(&refMap),
             new PassRepeated({new P4::ConstantFolding(&refMap, &typeMap),
                               new P4::StrengthReduction(&refMap, &typeMap)}),
             new P4::SimplifyComparisons(&refMap, &typeMap),
             new P4::CopyStructures(&refMap, &typeMap),
             new P4::LocalCopyPropagation(&refMap, &typeMap),
             new P4::SimplifySelectList(&refMap, &typeMap),
             new P4::MoveDeclarations(),  // more may have been introduced
             new P4::RemoveSelectBooleans(&refMap, &typeMap),
             new P4::SingleArgumentSelect(&refMap, &typeMap),
             new P4::ConstantFolding(&refMap, &typeMap),
             new P4::SimplifyControlFlow(&refMap, &typeMap), new P4::TableHit(&refMap, &typeMap),
             new P4::RemoveLeftSlices(&refMap, &typeMap), new EBPF::Lower(&refMap, &typeMap),
             evaluator, new P4::MidEndLast()});
        if (options.listMidendPasses) {
            midEnd.listPasses(*outStream, "\n");
            *outStream << std::endl;
            return nullptr;
        }
        if (options.excludeMidendPasses) {
            midEnd.removePasses(options.passesToExcludeMidend);
        }
    } else {
        midEnd.addPasses({new P4::ResolveReferences(&refMap),
                          new P4::TypeChecking(&refMap, &typeMap), evaluator});
    }
    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::errorCount() > 0) return nullptr;

    return evaluator->getToplevelBlock();
}
}  // namespace UBPF
