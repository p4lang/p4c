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

/**
 * \defgroup backend Backend
 * \brief Overview of backend passes
 *
 * The back-end performs a series of passes that can
 * possibly introduce some backtracking - returning to
 * a given point in the sequence of passes to re-run
 * it again, possibly with new information. This usually
 * happens when %PHV or %table allocation fails.
 *
 * The back-end starts with IR that was already transformed from front-end/mid-end IR
 * into back-end IR (via BackendConverter). Also the back-end works
 * on individual pipes in the program, not the entire program at once.
 * This means that the input of the back-end is a back-end IR
 * of a pipe and the output is a assembly file for this pipe.
 *
 * There are currently two top-level flows. Changing between them
 * can be done by setting options.alt_phv_alloc. These two flows are:
 * - Main flow:
 *   1. Run full %PHV allocation
 *   2. Run MAU allocation
 * - Alternative flow:
 *   1. Run trivial %PHV allocation
 *   2. Run MAU allocation
 *   3. Run full %PHV allocation
 * - Note: Both flows include different optimization/transformation
 *         passes in between and also possible backtracking.
 *
 * There are two special types of passes:
 * - Dumping passes - these passes dump some information from the compiler
 *                    usually some part of IR.
 *   - DumpPipe
 *   - DumpParser
 *   - DumpTableFlowGraph
 *   - DumpJsonGraph
 * - Logging passes - these passes create log/report files.
 *   - LogFlexiblePacking
 *   - CollectPhvLoggingInfo
 *   - GeneratePrimitiveInfo
 *   - LiveRangeReport
 *   - BFN::CollectIXBarInfo
 */

#include "backend.h"

#include <fstream>
#include <set>

#include "bf-p4c/arch/collect_hardware_constrained_fields.h"
#include "bf-p4c/common/alias.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/check_field_corruption.h"
#include "bf-p4c/common/check_for_unimplemented_features.h"
#include "bf-p4c/common/check_header_refs.h"
#include "bf-p4c/common/check_uninitialized_read.h"
#include "bf-p4c/common/elim_unused.h"
#include "bf-p4c/common/extract_maupipe.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/merge_pov_bits.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/common/size_of.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/logging/filelog.h"
#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/mau/adjust_byte_count.h"
#include "bf-p4c/mau/check_duplicate.h"
#include "bf-p4c/mau/dump_json_graph.h"
#include "bf-p4c/mau/empty_controls.h"
#include "bf-p4c/mau/finalize_mau_pred_deps_power.h"
#include "bf-p4c/mau/gen_prim_json.h"
#include "bf-p4c/mau/instruction_adjustment.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/mau/ixbar_info.h"
#include "bf-p4c/mau/ixbar_realign.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/mau_alloc.h"
#include "bf-p4c/mau/push_pop.h"
#include "bf-p4c/mau/selector_update.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/mau/validate_actions.h"
#include "bf-p4c/parde/add_metadata_pov.h"
#include "bf-p4c/parde/adjust_extract.h"
#include "bf-p4c/parde/check_parser_multi_write.h"
#include "bf-p4c/parde/clot/allocate_clot.h"
#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"
#include "bf-p4c/parde/collect_parser_usedef.h"
#include "bf-p4c/parde/infer_payload_offset.h"
#include "bf-p4c/parde/lower_parser.h"
#include "bf-p4c/parde/merge_parser_state.h"
#include "bf-p4c/parde/mirror/const_mirror_session_opt.h"
#include "bf-p4c/parde/reset_invalidated_checksum_headers.h"
#include "bf-p4c/parde/resolve_negative_extract.h"
#include "bf-p4c/parde/rewrite_parser_locals.h"
#include "bf-p4c/parde/stack_push_shims.h"
#include "bf-p4c/parde/update_parser_write_mode.h"
#include "bf-p4c/phv/add_alias_allocation.h"
#include "bf-p4c/phv/allocate_temps_and_finalize_liverange.h"
#include "bf-p4c/phv/analysis/dark.h"
#include "bf-p4c/phv/auto_init_metadata.h"
#include "bf-p4c/phv/check_unallocated.h"
#include "bf-p4c/phv/create_thread_local_instances.h"
#include "bf-p4c/phv/dump_table_flow_graph.h"
#include "bf-p4c/phv/finalize_stage_allocation.h"
#include "bf-p4c/phv/init_in_mau.h"
#include "bf-p4c/phv/phv_analysis.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"
#include "bf-p4c/phv/split_padding.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "bf-p4c/phv/v2/metadata_initialization.h"
#include "ir/ir-generated.h"
#include "ir/pass_manager.h"
#include "lib/indent.h"

namespace BFN {

/**
 * A class to collect all currently unimplemented features
 * and control whether we print an error (for debugging) or exit
 */
class CheckUnimplementedFeatures : public Inspector {
    bool _printAndNotExit;

 public:
    explicit CheckUnimplementedFeatures(bool print = false) : _printAndNotExit(print) {}

    bool preorder(const IR::EntriesList *entries) {
        auto source = entries->getSourceInfo().toPosition();
        if (_printAndNotExit)
            ::warning("Table entries (%s) are not yet implemented in this backend",
                      source.toString());
        else
            throw Util::CompilerUnimplemented(
                source.sourceLine, source.fileName,
                "Table entries are not yet implemented in this backend");
        return false;
    }
};

void force_link_dump(const IR::Node *n) { dump(n); }

static void debug_hook(const char *parent, unsigned idx, const char *pass, const IR::Node *n) {
    using namespace IndentCtl;

    if (LOGGING(5)) {
        const int pipeId = n->to<IR::BFN::Pipe>()->canon_id();
        Logging::FileLog fileLog(pipeId, "backend_passes.log"_cs);
        LOG5("PASS: " << pass << " [" << parent << " (" << idx << ")]:");
        dump(std::clog, n);
    } else {
        LOG4(pass << " [" << parent << " (" << idx << ")]:" << indent << endl
                  << *n << unindent << endl);
    }
}

Backend::Backend(const BFN_Options &o, int pipe_id)
    : options(o),
      uses(phv),
      clot(uses),
      defuse(phv),
      decaf(phv, uses, defuse, deps),
      table_summary(pipe_id, deps, phv, compilation_state),
      mau_backtracker(compilation_state, &table_summary),
      table_alloc(options, phv, deps, table_summary, &jsonGraph, mau_backtracker),
      parserHeaderSeqs(phv),
      longBranchDisabled() {
    BUG_CHECK(pipe_id >= 0, "Invalid pipe id in backend : %d", pipe_id);
    flexibleLogging = new LogFlexiblePacking(phv);
    phvLoggingInfo = new CollectPhvLoggingInfo(phv, uses, deps.red_info);
    phvLoggingDefUseInfo = options.debugInfo ? new PhvLogging::CollectDefUseInfo(defuse) : nullptr;
    auto *PHV_Analysis =
        new PHV_AnalysisPass(options, phv, uses, clot, defuse, deps, decaf, mau_backtracker,
                             phvLoggingInfo /*, &jsonGraph */, mauInitFields, table_summary);

    // Collect next table info if we're using LBs
    if (Device::numLongBranchTags() > 0 && !options.disable_long_branch) {
        nextTblProp.setVisitor(new JbayNextTable);
    } else {
        longBranchDisabled = true;
        nextTblProp.setVisitor(new DefaultNext(longBranchDisabled));
    }

    // Create even if Tofino, since this checks power is within limits.
    power_and_mpr = new MauPower::FinalizeMauPredDepsPower(phv, deps, &nextTblProp, options);

    liveRangeReport = new LiveRangeReport(phv, table_summary, defuse);
    auto *noOverlay = new PragmaNoOverlay(phv);
    auto *pragmaAlias = new PragmaAlias(phv, *noOverlay);

    auto *pragmaDoNotUseClot = new PragmaDoNotUseClot(phv);
    auto *allocateClot = Device::numClots() > 0 && options.use_clot
                             ? new AllocateClot(clot, phv, uses, *pragmaDoNotUseClot, *pragmaAlias)
                             : nullptr;

    addPasses({
        new DumpPipe("Initial table graph"),
        flexibleLogging,
        LOGGING(4) ? new DumpParser("begin_backend") : nullptr,
        new AdjustByteCountSetup,
        new CreateThreadLocalInstances,
        new CollectHardwareConstrainedFields,
        new CheckForUnimplementedFeatures(),
        new RemoveEmptyControls,
        new CatchBacktrack<LongBranchAllocFailed>([this] {
            if (!options.table_placement_long_branch_backtrack) {
                options.table_placement_long_branch_backtrack = true;
            } else {
                options.disable_long_branch = true;
                longBranchDisabled = true;
                nextTblProp.setVisitor(new DefaultNext(longBranchDisabled));
            }
            mau_backtracker.clear();
            table_summary.resetPlacement();
        }),
        new MultipleApply(options),
        new AddSelectorSalu,
        new FixupStatefulAlu,
        // CanonGatewayExpr checks gateway rows in table and tries to optimize
        // on gateway expressions. Since it can ellminate/modify condition
        // tables, this pass must run before PHV Analysis to ensure we do not
        // generate invalid metadata dependencies. Placing it early in backend,
        // as it will also error out on invalid gateway expressions and we fail
        // early in those cases.
        new CanonGatewayExpr,        // Must be before PHV_Analysis
        new CollectHeaderStackInfo,  // Needed by CollectPhvInfo.
        new CollectPhvInfo(phv),
        &defuse,
        Device::hasMetadataPOV() ? new AddMetadataPOV(phv) : nullptr,
        Device::currentDevice() == Device::TOFINO ? new ResetInvalidatedChecksumHeaders(phv)
                                                  : nullptr,
        new CollectPhvInfo(phv),
        &defuse,
        new CollectHeaderStackInfo,  // Needs to be rerun after CreateThreadLocalInstances, but
                                     // cannot be run after InstructionSelection.
        new RemovePushInitialization,
        new StackPushShims,
        new CollectPhvInfo(phv),  // Needs to be rerun after CreateThreadLocalInstances.
        new HeaderPushPop,
        new CollectPhvInfo(phv),
        new GatherReductionOrReqs(deps.red_info),
        new InstructionSelection(options, phv, deps.red_info),
        new DumpPipe("After InstructionSelection"_cs),
        new FindDependencyGraph(phv, deps, &options, "program_graph"_cs,
                                "After Instruction Selection"_cs),
        options.decaf ? &decaf : nullptr,
        new CollectPhvInfo(phv),
        // Collect pa_no_overlay pragmas to find conflicts with PragmaAlias and AutoAlias.
        // Conflicts with PragmaAlias will generate warnings.
        // Conflicts with AutoAlias will cancel AutoAlias for conflicting fields or field pairs
        noOverlay,
        // Aliasing replaces all uses of the alias source field with the alias destination field.
        // Therefore, run it first in the backend to ensure that all other passes use a union of the
        // constraints of the alias source and alias destination fields.
        pragmaAlias,
        new AutoAlias(phv, *pragmaAlias, *noOverlay),
        new Alias(phv, *pragmaAlias),
        new CollectPhvInfo(phv),
        new DumpPipe("After Alias"_cs),
        // This is the backtracking point from table placement to PHV allocation. Based on a
        // container conflict-free PHV allocation, we generate a number of no-pack conflicts between
        // fields (these are fields written in different nonmutually exclusive actions in the same
        // stage). As some of these no-pack conflicts may be related to bridged metadata fields, we
        // need to pull out the backtracking point from close to PHV allocation to before bridged
        // metadata packing.
        &mau_backtracker,
        new ResolveSizeOfOperator(),
        new DumpPipe("After ResolveSizeOfOperator"_cs),
        // Run after bridged metadata packing as bridged packing updates the parser state.
        new CollectPhvInfo(phv),
        new ParserCopyProp(phv),
        new RewriteParserMatchDefs(phv),
        new ResolveNegativeExtract,
        new CollectPhvInfo(phv),
        &defuse,
        ((Device::currentDevice() != Device::TOFINO) && options.infer_payload_offset)
            ? new InferPayloadOffset(phv, defuse)
            : nullptr,
        new CollectPhvInfo(phv),
        &defuse,
        new CollectHeaderStackInfo,
        new CollectPhvInfo(phv),
        new ValidToStkvalid(phv),  // Alias header stack $valid fields with $stkvalid slices.
                                   // Must happen before ElimUnused.
        new ConstMirrorSessionOpt(phv),
        new CollectPhvInfo(phv),
        &defuse,
        (options.no_deadcode_elimination == false) ? new ElimUnused(phv, defuse, zeroInitFields)
                                                   : nullptr,
        (options.no_deadcode_elimination == false) ? new ElimUnusedHeaderStackInfo : nullptr,
        (options.disable_parser_state_merging == false) ? new MergeParserStates : nullptr,
        options.auto_init_metadata ? nullptr : new DisableAutoInitMetadata(defuse, phv),
        // RemoveMetadataInits pass eliminates metadata field assignments which are set to zero as
        // hardware supports implicit zero initializations.
        //
        // While this is a good optimization strategy, in Tofino2 the initializations also come with
        // a separate assignment to set POV bit ($valid) for the field. This ensures the field is
        // deparsed. These POV bit assignments should not be eliminated (by ElimUnused) as that
        // would lead to the field not being deparsed and potentially incorrect program behavior.
        //
        // In order to track these fields whose POV bit assignments should not be
        // eliminated, we populate them in zeroInitFields within the pass.
        //
        // The zeroInitFields are then used to skip eliminating the POV assignments in ElimUnused
        // pass later on. The implicit zero initializations on these fields will ensure they are
        // deparsed as zero.
        &defuse,
        new RemoveMetadataInits(phv, defuse, zeroInitFields),
        new CollectPhvInfo(phv),
        &defuse,
        options.alt_phv_alloc_meta_init
            ? new PHV::v2::MetadataInitialization(mau_backtracker, phv, defuse)
            : nullptr,
        new CheckParserMultiWrite(phv),
        new CollectPhvInfo(phv),
        new CheckForHeaders(),
        pragmaDoNotUseClot,
        allocateClot,
        // Can't (re)run CollectPhvInfo after this point as it will corrupt clot allocation
        &defuse,
        // zeroInitFields populated in RemoveMetadataInits pass earlier will be used to skip
        // eliminating POV bit assignments associated with the fields within zeroInitFields.
        // Metadata POV bits are only added on Tofino2 hence this will not affect any Tofino1
        // programs
        (options.no_deadcode_elimination == false) ? new ElimUnused(phv, defuse, zeroInitFields)
                                                   : nullptr,
        new MergePovBits(phv),  // Ideally run earlier, see comment in class for explanation.

        // Two different allocation flow:
        // (1) Normal allocation:
        //                                if failed
        //     PHV alloc => table alloc ============> PHV alloc => ....
        // (2) Table placement first allocation:
        //                                        with table info
        //     Trivial PHV alloc => table alloc ==================> PHV alloc ===> redo table.
        new AddInitsInMAU(phv, mauInitFields, false),
        new DumpPipe("Before phv_analysis"_cs),
        new DumpTableFlowGraph(phv),
        options.alt_phv_alloc
            ? new PassManager({
                  // run trivial alloc for the first time
                  new PassIf(
                      [this]() {
                          auto actualState = table_summary.getActualState();
                          return actualState == State::ALT_INITIAL &&
                                 table_summary.getNumInvoked() == 0;
                      },
                      {
                          [=]() {
                              PHV_Analysis->set_trivial_alloc(true);
                              PHV_Analysis->set_no_code_change(true);
                              PHV_Analysis->set_physical_liverange_overlay(false);
                              PHV_Analysis->set_physical_stage_trivial(false);
                          },
                      }),
                  // run trivial alloc for the second time
                  new PassIf(
                      [this]() {
                          auto actualState = table_summary.getActualState();
                          return actualState == State::ALT_INITIAL &&
                                 table_summary.getNumInvoked() != 0;
                      },
                      {
                          [=]() {
                              PHV_Analysis->set_trivial_alloc(true);
                              PHV_Analysis->set_no_code_change(true);
                              PHV_Analysis->set_physical_liverange_overlay(false);
                              PHV_Analysis->set_physical_stage_trivial(true);
                          },
                      }),
                  // run actual PHV allocation
                  new PassIf(
                      [this]() {
                          auto actualState = table_summary.getActualState();
                          return actualState == State::ALT_FINALIZE_TABLE_SAME_ORDER ||
                                 actualState == State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED;
                      },
                      {
                          [=]() {
                              PHV_Analysis->set_trivial_alloc(false);
                              PHV_Analysis->set_no_code_change(true);
                              PHV_Analysis->set_physical_liverange_overlay(true);
                          },
                      }),
                  // ^^ three PassIf are mutex.
              })
            : new PassManager({
                  // Do PHV allocation.  Cannot run CollectPhvInfo afterwards, as that
                  // will clear the allocation.
                  [=]() {
                      PHV_Analysis->set_trivial_alloc(false);
                      PHV_Analysis->set_no_code_change(false);
                      PHV_Analysis->set_physical_liverange_overlay(false);
                  },
              }),
        PHV_Analysis,

        // CheckUninitializedReadAndOverlayedReads Pass checks all allocation slices for overlays
        // which can corrupt uninitialized read values and potentially cause fatal errors
        // while running tests.
        //
        // This pass cannot be called at the end of backend as there is a pass named
        // ReinstateAliasSources which will break the program's semantic correctness. Namely, some
        // uses will lose track of their defs, because variable names have been changed by some
        // passes e.g. ReinstateAliasSources. So this pass should be put at a place that is most
        // correct semantically.
        //
        // In addition, this pass has to be placed after PHV_Analysis, since pragmas information in
        // PHV_Analysis is needed.
        //
        // NOTE: This pass is an updated version of an older pass called
        // CheckUninitializedRead which did not check for overlays and reported all uninitialized
        // reads on fields as warnings. Uninitialized reads by themselves are not an issue unless
        // they are overlayed. Hence the older pass was modified to add the overlay check and only
        // report those reads.
        new CheckUninitializedAndOverlayedReads(defuse, phv, PHV_Analysis->get_pragmas(), options),
        new AddInitsInMAU(phv, mauInitFields, true),

        new SplitPadding(phv),
        allocateClot == nullptr ? nullptr : new ClotAdjuster(clot, phv),
        new ValidateActions(phv, deps.red_info, false, true, false),
        new PHV::AddAliasAllocation(phv),
        new ReinstateAliasSources(phv),  // revert AliasMembers/Slices to their original sources
        // This pass must be called before instruction adjustment since the primitive info
        // is per P4 actions. This should also happen before table placement which may cause
        // tables to be split across stages.
        new GeneratePrimitiveInfo(phv, primNode),
        &table_alloc,
        new DumpPipe("After TableAlloc"_cs),
        &table_summary,
        // Rerun defuse analysis here so that table placements are used to correctly calculate live
        // ranges output in the assembly.
        &defuse,
        options.alt_phv_alloc
            ? new PHV::AllocateTempsAndFinalizeLiverange(phv, clot, defuse, table_summary)
            : nullptr,
        liveRangeReport,
        new IXBarVerify(phv),
        new CollectIXBarInfo(phv),
        // DO NOT run CheckForUnallocatedTemps in table-first pass because temp vars has been
        // allocated in above AllocateTempsAndFinalizeLiverange.
        !options.alt_phv_alloc ? new CheckForUnallocatedTemps(phv, uses, clot, PHV_Analysis)
                               : nullptr,
        new InstructionAdjustment(phv, deps.red_info),
        &nextTblProp,  // Must be run after all modifications to the table graph have finished!
        new DumpPipe("Final table graph"_cs),
        new CheckFieldCorruption(defuse, phv, PHV_Analysis->get_pragmas()),
        new AdjustExtract(phv),
        phvLoggingDefUseInfo,
        // Update parser write modes to reflect SINGLE_WRITE + BITWISE_OR field merges
        new UpdateParserWriteMode(phv),
        // Rewrite parser and deparser IR to reflect the PHV allocation such that field operations
        // are converted into container operations.
        new LowerParser(phv, clot, defuse, parserHeaderSeqs, phvLoggingDefUseInfo),
        new CheckTableNameDuplicate,
        new CheckUnimplementedFeatures(options.allowUnimplemented),
        // must be called right before characterize power
        new FindDependencyGraph(phv, deps, &options, "placement_graph"_cs,
                                "After Table Placement"_cs),
        new DumpJsonGraph(deps, &jsonGraph, "After Table Placement"_cs, true),

        // Call this at the end of the backend. This changes the logical stages used for PHV
        // allocation to physical stages based on actual table placement.
        // DO NOT NEED to do it in alternative phv allocation because AllocSlices are already
        // physical stage based.
        Device::currentDevice() != Device::TOFINO && !options.alt_phv_alloc
            ? new FinalizeStageAllocation(phv, defuse, deps)
            : nullptr,

        // Must be called as last pass.  If the power budget is exceeded,
        // we cannot produce a binary file.
        power_and_mpr,
    });
    setName("Barefoot backend");

    if (options.excludeBackendPasses) removePasses(options.passesToExcludeBackend);

#if 0
    // check for passes that incorrectly duplicate shared attached tables
    // FIXME -- table placement currently breaks this because it does not rename attached
    // tables when it splits them across stages.
    auto *check = new CheckDuplicateAttached;
    addDebugHook([check](const char *, unsigned, const char *pass, const IR::Node *root) {
        check->pass = pass;
        root->apply(*check); }, true);
#endif

    if (LOGGING(4)) addDebugHook(debug_hook, true);
}

}  // namespace BFN
