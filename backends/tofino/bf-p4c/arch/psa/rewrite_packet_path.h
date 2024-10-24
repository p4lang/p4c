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
