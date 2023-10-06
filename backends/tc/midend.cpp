/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "midend.h"

namespace TC {

const IR::ToplevelBlock *MidEnd::run(TCOptions &options, const IR::P4Program *program,
                                     std::ostream *outStream) {
    if (program == nullptr && options.listMidendPasses == 0) return nullptr;
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager midEnd = {};
    midEnd.addPasses(
        {new P4::ConvertEnums(&refMap, &typeMap, new P4::EnumOn32Bits()),
         new P4::ClearTypeMap(&typeMap),
         new P4::RemoveMiss(&refMap, &typeMap),
         new P4::EliminateInvalidHeaders(&refMap, &typeMap),
         new P4::EliminateNewtype(&refMap, &typeMap),
         new P4::SimplifyControlFlow(&refMap, &typeMap),
         new P4::SimplifyKey(
             &refMap, &typeMap,
             new P4::OrPolicy(new P4::IsValid(&refMap, &typeMap), new P4::IsLikeLeftValue())),
         new P4::RemoveExits(&refMap, &typeMap),
         new P4::ConstantFolding(&refMap, &typeMap),
         new P4::SimplifySelectCases(&refMap, &typeMap, false),  // accept non-constant keysets
         new P4::ExpandEmit(&refMap, &typeMap),
         new P4::HandleNoMatch(&refMap),
         new P4::SimplifyParsers(&refMap),
         new PassRepeated({new P4::ConstantFolding(&refMap, &typeMap),
                           new P4::StrengthReduction(&refMap, &typeMap)}),
         new P4::SimplifyComparisons(&refMap, &typeMap),
         new P4::EliminateTuples(&refMap, &typeMap),
         new P4::SimplifySelectList(&refMap, &typeMap),
         new P4::MoveDeclarations(),  // more may have been introduced
         new P4::LocalCopyPropagation(&refMap, &typeMap),
         new PassRepeated({new P4::ConstantFolding(&refMap, &typeMap),
                           new P4::StrengthReduction(&refMap, &typeMap)}),
         new P4::RemoveSelectBooleans(&refMap, &typeMap),
         new P4::SingleArgumentSelect(&refMap, &typeMap),
         new P4::ConstantFolding(&refMap, &typeMap),
         new P4::SimplifyControlFlow(&refMap, &typeMap),
         new P4::TableHit(&refMap, &typeMap),
         new P4::RemoveLeftSlices(&refMap, &typeMap),
         new EBPF::Lower(&refMap, &typeMap),
         new P4::ParsersUnroll(true, &refMap, &typeMap),
         evaluator,
         new P4::MidEndLast()});
    if (options.listMidendPasses) {
        midEnd.listPasses(*outStream, "\n");
        *outStream << std::endl;
    }
    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::errorCount() > 0) return nullptr;

    return evaluator->getToplevelBlock();
}

}  // namespace TC
