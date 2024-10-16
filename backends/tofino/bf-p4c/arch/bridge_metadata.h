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

#ifndef BF_P4C_ARCH_BRIDGE_METADATA_H_
#define BF_P4C_ARCH_BRIDGE_METADATA_H_

#include "ir/ir.h"
#include "ir/namemap.h"
#include "lib/ordered_set.h"
#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "midend/eliminateSerEnums.h"

namespace BFN {

static const cstring META = "meta"_cs;
static const cstring COMPILER_META = "__bfp4c_compiler_generated_meta"_cs;
static const cstring BRIDGED_MD = "__bfp4c_bridged_metadata"_cs;
static const cstring BRIDGED_MD_HEADER = "__bfp4c_bridged_metadata_header"_cs;
static const cstring BRIDGED_MD_FIELD = "__bfp4c_fields"_cs;
static const cstring BRIDGED_MD_INDICATOR = "__bfp4c_bridged_metadata_indicator"_cs;

struct AddTnaBridgeMetadata : public PassManager {
    AddTnaBridgeMetadata(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            bool& program_uses_bridge_metadata);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_BRIDGE_METADATA_H_ */
