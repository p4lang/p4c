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

#ifndef BF_P4C_PARDE_ASM_OUTPUT_H_
#define BF_P4C_PARDE_ASM_OUTPUT_H_

#include "bf-p4c-options.h"
#include "bf-p4c/parde/parser_header_sequences.h"
#include "ir/ir.h"

class PhvInfo;
class ClotInfo;

/// \ingroup AsmOutput parde
///
/// Helper that can generate parser assembly and write it to an output stream.
struct ParserAsmOutput {
    ParserAsmOutput(const IR::BFN::Pipe *pipe, const PhvInfo &phv, const ClotInfo &clot_info,
                    gress_t gress);

 private:
    friend std::ostream &operator<<(std::ostream &, const ParserAsmOutput &);

    std::vector<const IR::BFN::BaseLoweredParser *> parsers;
    const PhvInfo &phv;
    const ClotInfo &clot_info;
};

/// \ingroup AsmOutput parde
///
/// Helper that can generate phase0 assembly and write it to an output stream.
struct Phase0AsmOutput {
    const IR::BFN::Pipe *pipe;
    const IR::BFN::Phase0 *phase0;
    Phase0AsmOutput(const IR::BFN::Pipe *pipe, const IR::BFN::Phase0 *phase0)
        : pipe(pipe), phase0(phase0) {}

 private:
    friend std::ostream &operator<<(std::ostream &, const Phase0AsmOutput &);
};

/// \ingroup AsmOutput parde
///
/// Helper that can generate deparser assembly and write it to an output stream.
struct DeparserAsmOutput {
    DeparserAsmOutput(const IR::BFN::Pipe *pipe, const PhvInfo &phv, const ClotInfo &clot, gress_t);

    const PhvInfo &phv;
    const ClotInfo &clot;

 private:
    friend std::ostream &operator<<(std::ostream &, const DeparserAsmOutput &);

    const IR::BFN::LoweredDeparser *deparser;
};

#endif /* BF_P4C_PARDE_ASM_OUTPUT_H_ */
