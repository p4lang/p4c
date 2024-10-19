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

#ifndef BF_P4C_ARCH_PSA_REWRITE_BRIDGE_METADATA_H_
#define BF_P4C_ARCH_PSA_REWRITE_BRIDGE_METADATA_H_

#include "bf-p4c/arch/psa/programStructure.h"
#include "ir/ir.h"

namespace BFN {

struct AddPsaBridgeMetadata : public PassManager {
    AddPsaBridgeMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                         PSA::ProgramStructure *structure);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_PSA_REWRITE_BRIDGE_METADATA_H_ */
