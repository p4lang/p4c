/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_ARCH_BRIDGE_METADATA_H_
#define BF_P4C_ARCH_BRIDGE_METADATA_H_

#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"
#include "ir/namemap.h"
#include "lib/ordered_set.h"
#include "midend/eliminateSerEnums.h"

namespace BFN {

static const cstring META = "meta"_cs;
static const cstring COMPILER_META = "__bfp4c_compiler_generated_meta"_cs;
static const cstring BRIDGED_MD = "__bfp4c_bridged_metadata"_cs;
static const cstring BRIDGED_MD_HEADER = "__bfp4c_bridged_metadata_header"_cs;
static const cstring BRIDGED_MD_FIELD = "__bfp4c_fields"_cs;
static const cstring BRIDGED_MD_INDICATOR = "__bfp4c_bridged_metadata_indicator"_cs;

struct AddTnaBridgeMetadata : public PassManager {
    AddTnaBridgeMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                         bool &program_uses_bridge_metadata);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_BRIDGE_METADATA_H_ */
