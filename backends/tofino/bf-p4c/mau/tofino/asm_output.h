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
