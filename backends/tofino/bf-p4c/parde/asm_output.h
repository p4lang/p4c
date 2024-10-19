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
