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

namespace P4::TC {

const IR::ToplevelBlock *MidEnd::run(TCOptions &options, const IR::P4Program *program,
                                     std::ostream *outStream) {
    if (program == nullptr && options.listMidendPasses == 0) return nullptr;
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager midEnd = {};
    midEnd.addPasses({
        new P4::ConvertEnums(&typeMap, new P4::EnumOn32Bits()),
        new P4::ClearTypeMap(&typeMap),
        new P4::RemoveMiss(&typeMap),
        new P4::EliminateInvalidHeaders(&typeMap),
        new P4::EliminateNewtype(&typeMap),
        new P4::EliminateSerEnums(&typeMap),
        new P4::SimplifyControlFlow(&typeMap),
        new P4::SimplifyKey(&typeMap,
                            new P4::OrPolicy(new P4::IsValid(&typeMap), new P4::IsLikeLeftValue())),
        new P4::RemoveExits(&typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifySelectCases(&typeMap, false),  // accept non-constant keysets
        new P4::ExpandEmit(&typeMap),
        new P4::HandleNoMatch(),
        new P4::SimplifyParsers(),
        new PassRepeated({
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::StrengthReduction(&typeMap),
        }),
        new P4::SimplifyComparisons(&typeMap),
        new P4::EliminateTuples(&typeMap),
        new P4::SimplifySelectList(&typeMap),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::LocalCopyPropagation(&refMap, &typeMap),
        new PassRepeated({
            new P4::ConstantFolding(&refMap, &typeMap),
            new P4::StrengthReduction(&typeMap),
        }),
        new P4::RemoveSelectBooleans(&typeMap),
        new P4::SingleArgumentSelect(&typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&typeMap),
        new P4::TableHit(&typeMap),
        new P4::RemoveLeftSlices(&typeMap),
        new EBPF::Lower(&refMap, &typeMap),
        new P4::ParsersUnroll(true, &refMap, &typeMap),
        evaluator,
        new P4::MidEndLast(),
    });
    if (options.listMidendPasses) {
        midEnd.listPasses(*outStream, cstring::newline);
        *outStream << std::endl;
    }
    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::P4::errorCount() > 0) return nullptr;

    return evaluator->getToplevelBlock();
}

}  // namespace P4::TC
