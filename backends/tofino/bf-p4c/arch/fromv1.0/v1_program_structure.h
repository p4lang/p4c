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
