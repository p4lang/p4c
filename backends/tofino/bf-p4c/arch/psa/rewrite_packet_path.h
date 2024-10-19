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

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_PSA_REWRITE_PACKET_PATH_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_PSA_REWRITE_PACKET_PATH_H_

#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/arch/psa/programStructure.h"
#include "bf-p4c/arch/psa/psa.h"
#include "ir/ir.h"

namespace BFN {

namespace PSA {

struct RewritePacketPath : public PassManager {
    explicit RewritePacketPath(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                               PSA::ProgramStructure *structure);
};

}  // namespace PSA

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_PSA_REWRITE_PACKET_PATH_H_ */
