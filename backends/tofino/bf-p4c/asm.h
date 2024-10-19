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

#ifndef _BACKENDS_TOFINO_BF_P4C_ASM_H_
#define _BACKENDS_TOFINO_BF_P4C_ASM_H_

#include <string>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/mau/asm_output.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/tofino/asm_output.h"
#include "bf-p4c/parde/asm_output.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parser_header_sequences.h"
#include "bf-p4c/phv/asm_output.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/live_range_report.h"

class FieldDefUse;

namespace BFN {

/**
 * \defgroup AsmOutput Assembly output
 *
 * \brief Generate assembly output
 *
 * Generate the assembly output for the program. Invokes separate subpasses for outputting %PHV,
 * parser, deparser, and %MAU sections.
 */

/**
 * \ingroup AsmOutput
 *
 * \brief Generate assembly output
 *
 * Generate the assembly output for the program. Invokes separate subpasses for outputting %PHV,
 * parser, deparser, and %MAU sections.
 */
class AsmOutput : public Inspector {
 private:
    int pipe_id;
    const PhvInfo &phv;
    const ClotInfo &clot;
    const FieldDefUse &defuse;
    const LogRepackedHeaders *flex;
    const NextTable *nxt_tbl;
    const MauPower::FinalizeMauPredDepsPower *power_and_mpr;
    const TableSummary &tbl_summary;
    const LiveRangeReport *live_range_report;
    const ParserHeaderSequences &prsr_header_seqs;
    const BFN_Options &options;
    /// Tell this pass whether it is called after a succesful compilation
    bool _successfulCompile = true;
    std::string ghostPhvContainer() const;

 public:
    AsmOutput(int pipe_id, const PhvInfo &phv, const ClotInfo &clot, const FieldDefUse &defuse,
              const LogRepackedHeaders *flex, const NextTable *nxts,
              const MauPower::FinalizeMauPredDepsPower *pmpr, const TableSummary &tbl_summary,
              const LiveRangeReport *live_range_report,
              const ParserHeaderSequences &prsr_header_seqs, const BFN_Options &opts, bool success);

    bool preorder(const IR::BFN::Pipe *pipe) override;
};

} /* namespace BFN */

#endif /* _BACKENDS_TOFINO_BF_P4C_ASM_H_ */
