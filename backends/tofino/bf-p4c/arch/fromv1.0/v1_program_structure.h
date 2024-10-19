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

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_V1_PROGRAM_STRUCTURE_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_V1_PROGRAM_STRUCTURE_H_

#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/ir/gress.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"
#include "ir/namemap.h"
#include "lib/ordered_set.h"

namespace BFN {

namespace V1 {

/// Experimental implementation of programStructure to facilitate the
/// translation between P4-16 program of different architecture.
struct ProgramStructure : BFN::ProgramStructure {
    /// user program specific info
    cstring type_h;
    cstring type_m;
    const IR::Parameter *user_metadata;
    bool backward_compatible = false;

    void createParsers() override;
    void createControls() override;
    void createMain() override;
    void createPipeline();
    const IR::P4Program *create(const IR::P4Program *program) override;
};

}  // namespace V1

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_V1_PROGRAM_STRUCTURE_H_ */
