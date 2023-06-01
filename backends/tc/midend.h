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

#ifndef BACKENDS_TC_MIDEND_H_
#define BACKENDS_TC_MIDEND_H_

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
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "midend/actionSynthesis.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/noMatch.h"
#include "midend/parserUnroll.h"
#include "midend/removeExits.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/singleArgumentSelect.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"
#include "options.h"

namespace TC {

class MidEnd {
 public:
    std::vector<DebugHook> hooks;
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    // If p4c is run with option '--listMidendPasses', outStream is used for printing passes names
    const IR::ToplevelBlock *run(TCOptions &options, const IR::P4Program *program,
                                 std::ostream *outStream = nullptr);
};
}  // namespace TC

#endif /* BACKENDS_TC_MIDEND_H_ */
