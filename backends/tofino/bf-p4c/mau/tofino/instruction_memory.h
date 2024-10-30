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

#ifndef BF_P4C_MAU_TOFINO_INSTRUCTION_MEMORY_H_
#define BF_P4C_MAU_TOFINO_INSTRUCTION_MEMORY_H_

#include "bf-p4c/mau/instruction_memory.h"

namespace Tofino {

struct InstructionMemory : public ::InstructionMemory {
    static constexpr int IMEM_ROWS = 32;
    static constexpr int IMEM_COLORS = 2;

    BFN::Alloc2D<cstring, IMEM_ROWS, IMEM_COLORS> ingress_imem_use;
    BFN::Alloc2D<cstring, IMEM_ROWS, IMEM_COLORS> egress_imem_use;

    BFN::Alloc2D<bitvec, IMEM_ROWS, IMEM_COLORS> ingress_imem_slot_inuse;
    BFN::Alloc2D<bitvec, IMEM_ROWS, IMEM_COLORS> egress_imem_slot_inuse;

    BFN::Alloc2Dbase<cstring> &imem_use(gress_t gress) {
        if (gress == INGRESS || gress == GHOST) return ingress_imem_use;
        return egress_imem_use;
    }

    BFN::Alloc2Dbase<bitvec> &imem_slot_inuse(gress_t gress) {
        if (gress == INGRESS || gress == GHOST) return ingress_imem_slot_inuse;
        return egress_imem_slot_inuse;
    }

    InstructionMemory() : ::InstructionMemory(Device::imemSpec()) {}
};

}  // end namespace Tofino

#endif /* BF_P4C_MAU_TOFINO_INSTRUCTION_MEMORY_H_ */
