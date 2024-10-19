/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/arch/tna.h"

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/arch/check_extern_invocation.h"
#include "bf-p4c/arch/fromv1.0/phase0.h"
#include "bf-p4c/arch/rewrite_action_selector.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/parde/field_packing.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "midend/copyStructures.h"
#include "midend/validateProperties.h"

namespace BFN {

TnaArchTranslation::TnaArchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                       BFN_Options &options) {
    setName("TnaArchTranslation");
    addDebugHook(options.getDebugHook());
    addPasses({
        new RewriteControlAndParserBlocks(refMap, typeMap),
        new RestoreParams(options, refMap, typeMap),
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
        new CheckTNAExternInvocation(refMap, typeMap),
        new LoweringType(),
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
        new P4::ValidateTableProperties(
            {"implementation"_cs, "size"_cs, "counters"_cs, "meters"_cs, "filters"_cs,
             "idle_timeout"_cs, "registers"_cs, "requires_versioning"_cs, "atcam"_cs, "alpm"_cs,
             "proxy_hash"_cs,
             /* internal table property, not exposed to customer */
             "as_alpm"_cs, "number_partitions"_cs, "subtrees_per_partition"_cs,
             "atcam_subset_width"_cs, "shift_granularity"_cs}),
        new BFN::RewriteActionSelector(refMap, typeMap),
        new ConvertPhase0(refMap, typeMap),
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
    });
}

}  // namespace BFN
