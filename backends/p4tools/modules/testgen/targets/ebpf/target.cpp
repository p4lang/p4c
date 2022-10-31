#include "backends/p4tools/modules/testgen/targets/ebpf/target.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"
#include "p4tools/common/core/solver.h"

#include "backends/p4tools/testgen/lib/namespace_context.h"
#include "backends/p4tools/testgen/targets/ebpf/test_backend.h"
#include "p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "p4tools/testgen/core/program_info.h"
#include "p4tools/testgen/core/target.h"
#include "p4tools/testgen/lib/execution_state.h"
#include "p4tools/testgen/targets/ebpf/cmd_stepper.h"
#include "p4tools/testgen/targets/ebpf/expr_stepper.h"
#include "p4tools/testgen/targets/ebpf/program_info.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

/* =============================================================================================
 *  EBPFTestgenTarget implementation
 ============================================================================================= */

EBPFTestgenTarget::EBPFTestgenTarget() : TestgenTarget("ebpf", "ebpf") {}

void EBPFTestgenTarget::make() {
    static EBPFTestgenTarget* INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new EBPFTestgenTarget();
    }
}

const EBPFProgramInfo* EBPFTestgenTarget::initProgram_impl(
    const IR::P4Program* program, const IR::Declaration_Instance* mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto* ns = NamespaceContext::Empty->push(program);
    std::vector<const IR::Type_Declaration*> blocks;
    argumentsToTypeDeclarations(ns, mainDecl->arguments, blocks);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 2, "%1%: The EBPF architecture requires 2 blocks. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration*> programmableBlocks;
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto* declType = blocks.at(idx);
        auto canonicalName = archSpec.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    // TODO: We bound the max packet size. However, eBPF should be able to support jumbo sized
    // packets. There might be a bug in the framework
    auto& testgenOptions = TestgenOptions::get();
    if (testgenOptions.maxPktSize > 12000) {
        ::warning("Max packet size %1% larger than 12000 bits. Bounding size to 12000 bits.",
                  testgenOptions.maxPktSize);
        testgenOptions.maxPktSize = 12000;
    }

    return new EBPFProgramInfo(program, programmableBlocks);
}

EBPFTestBackend* EBPFTestgenTarget::getTestBackend_impl(const ProgramInfo& programInfo,
                                                        ExplorationStrategy& symbex,
                                                        const boost::filesystem::path& testPath,
                                                        boost::optional<uint32_t> seed) const {
    return new EBPFTestBackend(programInfo, symbex, testPath, seed);
}

int EBPFTestgenTarget::getPortNumWidth_bits_impl() const { return 9; }

EBPFCmdStepper* EBPFTestgenTarget::getCmdStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                                      const ProgramInfo& programInfo) const {
    return new EBPFCmdStepper(state, solver, programInfo);
}

EBPFExprStepper* EBPFTestgenTarget::getExprStepper_impl(ExecutionState& state,
                                                        AbstractSolver& solver,
                                                        const ProgramInfo& programInfo) const {
    return new EBPFExprStepper(state, solver, programInfo);
}

const ArchSpec EBPFTestgenTarget::archSpec =
    ArchSpec("ebpfFilter", {// parser parse<H>(packet_in packet, out H headers);
                            {"parse", {nullptr, "*hdr"}},
                            // control filter<H>(inout H headers, out bool accept);
                            {"filter", {"*hdr", "*accept"}}});

const ArchSpec* EBPFTestgenTarget::getArchSpecImpl() const { return &archSpec; }

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
