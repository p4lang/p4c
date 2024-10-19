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

#ifndef BF_P4C_MAU_TOFINO_ASM_OUTPUT_H_
#define BF_P4C_MAU_TOFINO_ASM_OUTPUT_H_

#include "bf-p4c/mau/asm_output.h"

namespace Tofino {

using namespace P4;

class MauAsmOutput : public ::MauAsmOutput {
 private:
    void emit_table_format(std::ostream &out, indent_t, const TableFormat::Use &use,
                           const TableMatch *tm, bool ternary, bool no_match) const;
    void emit_memory(std::ostream &out, indent_t, const Memories::Use &,
                     const IR::MAU::Table::Layout *l = nullptr,
                     const TableFormat::Use *f = nullptr) const;

 public:
    MauAsmOutput(const PhvInfo &phv, const IR::BFN::Pipe *pipe, const NextTable *nxts,
                 const MauPower::FinalizeMauPredDepsPower *pmpr, const BFN_Options &options)
        : ::MauAsmOutput(phv, pipe, nxts, pmpr, options) {}
};

}  // end namespace Tofino

#endif /* BF_P4C_MAU_TOFINO_ASM_OUTPUT_H_ */
