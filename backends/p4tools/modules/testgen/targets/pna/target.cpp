#include "backends/p4tools/modules/testgen/targets/pna/target.h"

#include <stddef.h>

#include <map>
#include <string>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/namespace_context.h"
#include "backends/p4tools/modules/testgen/targets/pna/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"
#include "backends/p4tools/modules/testgen/targets/pna/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_backend.h"

namespace P4Tools::P4Testgen::Pna {

/* =============================================================================================
 *  PnaDpdkTestgenTarget implementation
 * ============================================================================================= */

PnaDpdkTestgenTarget::PnaDpdkTestgenTarget() : TestgenTarget("dpdk", "pna") {}

void PnaDpdkTestgenTarget::make() {
    static PnaDpdkTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new PnaDpdkTestgenTarget();
    }
}

const PnaDpdkProgramInfo *PnaDpdkTestgenTarget::initProgram_impl(
    const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto *ns = NamespaceContext::Empty->push(program);
    std::vector<const IR::Type_Declaration *> blocks;
    argumentsToTypeDeclarations(ns, mainDecl->arguments, blocks);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 4, "%1%: The PNA architecture requires 4 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = archSpec.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new PnaDpdkProgramInfo(program, programmableBlocks);
}

PnaTestBackend *PnaDpdkTestgenTarget::getTestBackend_impl(const ProgramInfo &programInfo,
                                                          SymbolicExecutor &symbex,
                                                          const boost::filesystem::path &testPath,
                                                          boost::optional<uint32_t> seed) const {
    return new PnaTestBackend(programInfo, symbex, testPath, seed);
}

int PnaDpdkTestgenTarget::getPortNumWidth_bits_impl() const { return 9; }

PnaDpdkCmdStepper *PnaDpdkTestgenTarget::getCmdStepper_impl(ExecutionState &state,
                                                            AbstractSolver &solver,
                                                            const ProgramInfo &programInfo) const {
    return new PnaDpdkCmdStepper(state, solver, programInfo);
}

PnaDpdkExprStepper *PnaDpdkTestgenTarget::getExprStepper_impl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new PnaDpdkExprStepper(state, solver, programInfo);
}

const ArchSpec PnaDpdkTestgenTarget::archSpec = ArchSpec(
    "PNA_NIC", {
                   // parser MainParserT<MH, MM>(
                   //     packet_in pkt,
                   //     //in    PM pre_user_meta,
                   //     out   MH main_hdr,
                   //     inout MM main_user_meta,
                   //     in    pna_main_parser_input_metadata_t istd);
                   {"MainParserT", {nullptr, "*main_hdr", "*main_user_meta", "*istd"}},
                   // control PreControlT<PH, PM>(
                   //     in    PH pre_hdr,
                   //     inout PM pre_user_meta,
                   //     in    pna_pre_input_metadata_t  istd,
                   //     inout pna_pre_output_metadata_t ostd);
                   {"PreControlT", {"*main_hdr", "*main_user_meta", "*pre_istd", "*pre_ostd"}},
                   // control MainControlT<MH, MM>(
                   //     //in    PM pre_user_meta,
                   //     inout MH main_hdr,
                   //     inout MM main_user_meta,
                   //     in    pna_main_input_metadata_t  istd,
                   //     inout pna_main_output_metadata_t ostd);
                   {"MainControlT", {"*main_hdr", "*main_user_meta", "*istd", "*ostd"}},
                   // control MainDeparserT<MH, MM>(
                   //     packet_out pkt,
                   //     in    MH main_hdr,
                   //     in    MM main_user_meta,
                   //     in    pna_main_output_metadata_t ostd);
                   {"MainDeparserT", {nullptr, "*main_hdr", "*main_user_meta", "*ostd"}},
               });

const ArchSpec *PnaDpdkTestgenTarget::getArchSpecImpl() const { return &archSpec; }

}  // namespace P4Tools::P4Testgen::Pna
