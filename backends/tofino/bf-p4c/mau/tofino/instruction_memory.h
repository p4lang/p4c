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
        if (gress == INGRESS || gress == GHOST)
            return ingress_imem_use;
        return egress_imem_use;
    }

    BFN::Alloc2Dbase<bitvec> &imem_slot_inuse(gress_t gress) {
        if (gress == INGRESS || gress == GHOST)
            return ingress_imem_slot_inuse;
        return egress_imem_slot_inuse;
    }

    InstructionMemory() : ::InstructionMemory(Device::imemSpec()) {}
};

}  // end namespace Tofino

#endif /* BF_P4C_MAU_TOFINO_INSTRUCTION_MEMORY_H_ */
