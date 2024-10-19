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

#include "bf-p4c/asm.h"

#include <sys/stat.h>

#include <climits>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

// #include "bf-asm/version.h"
#include "bf-p4c/common/run_id.h"
#include "bf-p4c/device.h"

namespace BFN {

std::string AsmOutput::ghostPhvContainer() const {
    const std::map<cstring, PHV::Field> &fields = phv.get_all_fields();
    std::set<PHV::Container> ghost_containers;
    std::stringstream output;
    for (auto &kv : fields) {
        const PHV::Field &field = kv.second;
        // TODO: Do we ever have ghost fields what are wider than a container?
        // If so, then we'd only pick up the container of the LSB here.
        if (field.name.startsWith("ghost::gh_intr_md") && !field.pov)
            ghost_containers.insert(field.for_bit(0).container());
    }
    if (!ghost_containers.empty()) {
        if (ghost_containers.size() > 1) {
            output << "[ ";
        }
        std::string sep;
        for (const auto &cnt : ghost_containers) {
            output << sep << cnt;
            sep = ", ";
        }
        if (ghost_containers.size() > 1) {
            output << " ]";
        }
    } else {
        BUG("No allocation for ghost metadata?");
    }
    return output.str();
}

AsmOutput::AsmOutput(int pipe_id, const PhvInfo &phv, const ClotInfo &clot,
                     const FieldDefUse &defuse, const LogRepackedHeaders *flex,
                     const NextTable *nxts, const MauPower::FinalizeMauPredDepsPower *pmpr,
                     const TableSummary &tbl_summary, const LiveRangeReport *live_range_report,
                     const ParserHeaderSequences &prsr_header_seqs, const BFN_Options &opts,
                     bool success)
    : pipe_id(pipe_id),
      phv(phv),
      clot(clot),
      defuse(defuse),
      flex(flex),
      nxt_tbl(nxts),
      power_and_mpr(pmpr),
      tbl_summary(tbl_summary),
      live_range_report(live_range_report),
      prsr_header_seqs(prsr_header_seqs),
      options(opts),
      _successfulCompile(success) {}

bool AsmOutput::preorder(const IR::BFN::Pipe *pipe) {
    LOG1("ASM generation for successful compile? " << (_successfulCompile ? "true" : "false"));

    if (_successfulCompile) {
        auto outputDir = BFNContext::get().getOutputDirectory(cstring::empty, pipe_id);
        if (!outputDir) return false;
        cstring outputFile = outputDir + "/"_cs + options.programName + ".bfa"_cs;
        std::ofstream out(outputFile, std::ios_base::out);

        MauAsmOutput *mauasm = nullptr;
        if (!mauasm) mauasm = new Tofino::MauAsmOutput(phv, pipe, nxt_tbl, power_and_mpr, options);

        pipe->apply(*mauasm);

        out << "version:"
            << std::endl
            // << "  version: " << BFASM::Version::getVersion() << std::endl
            << "  run_id: \"" << RunId::getId() << "\"" << std::endl
            << "  target: " << Device::name() << std::endl;
        // set the default error mode used by all stages
        // FIXME should have a way to control this from the P4 source
        out << "error_mode: propagate_and_disable" << std::endl;
        if (::errorCount() == 0) {
            out << PhvAsmOutput(phv, defuse, tbl_summary, live_range_report,
                                pipe->ghost_thread.ghost_parser != nullptr);
            out << ParserAsmOutput(pipe, phv, clot, INGRESS);
            out << DeparserAsmOutput(pipe, phv, clot, INGRESS);
            if (pipe->ghost_thread.ghost_parser != nullptr) {
                out << "parser ghost: " << std::endl;
                out << "  ghost_md: " << ghostPhvContainer() << std::endl;
                if (ghost_only_on_other_pipes(pipe_id)) {
                    // In future this may be tied to a
                    // command line option which dictates the pipe mask
                    // value
                    out << "  pipe_mask: 0" << std::endl;
                }
            }
            out << ParserAsmOutput(pipe, phv, clot, EGRESS);
            out << DeparserAsmOutput(pipe, phv, clot, EGRESS) << *mauasm << std::endl
                << flex->asm_output() << std::endl;
        }
        out << std::flush;
    }
    return false;
}

}  // namespace BFN
