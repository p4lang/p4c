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

#include "bf-p4c/parde/lower_parser.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/optional/optional_io.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/debug_info.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/parde/allocate_parser_checksum.h"
#include "bf-p4c/parde/allocate_parser_match_register.h"
#include "bf-p4c/parde/characterize_parser.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/coalesce_learning.h"
#include "bf-p4c/parde/collect_parser_usedef.h"
#include "bf-p4c/parde/common/allocators.h"
#include "bf-p4c/parde/common/match_reducer.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/field_packing.h"
#include "bf-p4c/parde/lowered/compute_buffer_requirements.h"
#include "bf-p4c/parde/lowered/compute_init_zero_containers.h"
#include "bf-p4c/parde/lowered/compute_lowered_deparser_ir.h"
#include "bf-p4c/parde/lowered/compute_lowered_parser_ir.h"
#include "bf-p4c/parde/lowered/compute_multi_write_containers.h"
#include "bf-p4c/parde/lowered/eliminate_empty_states.h"
#include "bf-p4c/parde/lowered/helpers.h"
#include "bf-p4c/parde/lowered/hoist_common_match_operations.h"
#include "bf-p4c/parde/lowered/merge_lowered_parser_states.h"
#include "bf-p4c/parde/lowered/rewrite_emit_clot.h"
#include "bf-p4c/parde/lowered/split_greedy_parser_states.h"
#include "bf-p4c/parde/parde_utils.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/split_parser_state.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"
#include "lib/safe_vector.h"
#include "lib/stringref.h"

namespace Parde::Lowered {

using namespace P4;

/**
 * \ingroup LowerParserIR
 * \brief The pass that replaces an IR::BRN::Parser node with an IR::BFN::LoweredParser node.
 *
 * Replace the high-level parser IR version of each parser's root node with its
 * lowered version. This has the effect of replacing the entire parse graph.
 */
struct ReplaceParserIR : public Transform {
    explicit ReplaceParserIR(const ComputeLoweredParserIR &computed) : computed(computed) {}

 private:
    const IR::BFN::LoweredParser *preorder(IR::BFN::Parser *parser) override {
        BUG_CHECK(computed.loweredStates.find(parser->start) != computed.loweredStates.end(),
                  "Didn't lower the start state?");
        prune();

        auto *loweredParser =
            new IR::BFN::LoweredParser(parser->gress, computed.loweredStates.at(parser->start),
                                       parser->phase0, parser->name, parser->portmap);

        if (parser->gress == INGRESS) {
            loweredParser->hdrLenAdj = Device::pardeSpec().byteTotalIngressMetadataSize();
        } else {
            loweredParser->metaOpt = computed.egressMetaOpt;
            loweredParser->hdrLenAdj = computed.egressMetaSize;
        }

        if (parser->gress == INGRESS && computed.igParserError)
            loweredParser->parserError = computed.igParserError;
        else if (parser->gress == EGRESS && computed.egParserError)
            loweredParser->parserError = computed.egParserError;

        return loweredParser;
    }

    const ComputeLoweredParserIR &computed;
};

/**
 * \ingroup LowerParserIR
 * \brief Resolves constants in parser.
 */
class ResolveParserConstants : public ParserTransform {
    const PhvInfo &phv;
    const ClotInfo &clotInfo;

    IR::BFN::ConstantRVal *preorder(IR::BFN::TotalContainerSize *tcs) override {
        IR::Vector<IR::BFN::ContainerRef> containers;
        std::vector<Clot *> clots;

        std::tie(containers, clots) = lowerFields(phv, clotInfo, tcs->fields);

        BUG_CHECK(clots.empty(), "Fields allocated to CLOT?");

        std::map<PHV::Container, std::set<unsigned>> containerBytes;

        for (auto cr : containers) {
            unsigned loByte = 0, hiByte = cr->container.size() / 8;

            if (cr->range) {
                auto range = *(cr->range);
                loByte = range.lo / 8;
                hiByte = range.hi / 8;
            }

            for (unsigned i = loByte; i <= hiByte; i++) {
                containerBytes[cr->container].insert(i);
            }
        }

        unsigned totalBytes = 0;

        for (auto cb : containerBytes) totalBytes += cb.second.size();

        auto rv = new IR::BFN::ConstantRVal(totalBytes);
        return rv;
    }

 public:
    ResolveParserConstants(const PhvInfo &phv, const ClotInfo &clotInfo)
        : phv(phv), clotInfo(clotInfo) {}
};

/**
 * \ingroup LowerParserIR
 * \brief Find all of the states that do a checksum deposit but also do not extract/shift
 *        before doing it (= the end_pos is negative) and also all of the states from which
 *        we can get via 0 shift to a state that does this negative checksum deposit.
 */
struct FindNegativeDeposits : public ParserInspector {
 public:
    std::set<cstring> states_to_shift;

    FindNegativeDeposits() {}

 private:
    // Find any csum deposit with negative transition and add it's parser state
    void postorder(const IR::BFN::ChecksumResidualDeposit *crd) override {
        auto ps = findContext<IR::BFN::ParserState>();
        if (!ps) return;
        if (crd->header_end_byte && crd->header_end_byte->range.lo < 0) {
            LOG5("FindNegativeDeposits: State "
                 << ps->name << " tagged to be updated as it does negative deposit.");
            states_to_shift.insert(ps->name);
        }
    }

    // Tag also all of the parser states with zero shift leading to an added state
    // (to find and include every state after the last extract)
    void postorder(const IR::BFN::Transition *t) override {
        if (t->shift != 0 || !t->next || !states_to_shift.count(t->next->name)) return;
        auto ps = findContext<IR::BFN::ParserState>();
        if (!ps) return;
        LOG5("FindZeroShiftDeposits: State "
             << ps->name
             << " tagged to be updated since we can get to a negative deposit via 0 shift.");
        states_to_shift.insert(ps->name);
    }
};

/**
 * \ingroup LowerParserIR
 * \brief Updates the IR so that every checksum deposit can also shift by at least one.
 *
 * To do this for any state that does a negative deposit we change the shift
 * to 1 and decrease any shifts in the states leading into this one.
 */
struct RemoveNegativeDeposits : public ParserTransform {
    const FindNegativeDeposits &found;

    explicit RemoveNegativeDeposits(const FindNegativeDeposits &found) : found(found) {}

 private:
    // Update any transition leading to the tagged state to leave 1 byte
    IR::Node *preorder(IR::BFN::Transition *tr) {
        if (tr->shift == 0 || !tr->next || !found.states_to_shift.count(tr->next->name)) return tr;
        auto new_tr = tr->clone();
        new_tr->shift = tr->shift - 1;
        auto ps = findContext<IR::BFN::ParserState>();
        LOG4("RemoveNegativeDeposits: Transition from "
             << (ps ? ps->name : "--unknown--") << " to " << tr->next->name
             << " changed shift: " << tr->shift << "->" << new_tr->shift);
        return new_tr;
    }

    // Shift PacketRVals in every state that was found by one byte
    // Also update transitions from this state that lead outside of the tagged path
    // to shift by one more byte (to get rid of the extra 1 byte left over)
    IR::Node *preorder(IR::BFN::ParserState *ps) {
        if (!found.states_to_shift.count(ps->name)) return ps;
        const int shift_amt = -8;
        auto new_ps = ps->clone();
        LOG4("RemoveNegativeDeposits: Shifting every PacketRVal in state " << ps->name
                                                                           << " by one byte.");
        new_ps->statements = *(new_ps->statements.apply(ShiftPacketRVal(shift_amt)));
        new_ps->selects = *(new_ps->selects.apply(ShiftPacketRVal(shift_amt)));
        IR::Vector<IR::BFN::Transition> new_transitions;
        for (auto tr : ps->transitions) {
            auto new_tr = tr->clone();
            // Decrease only shifts leading outside of the tagged path
            // Inside the tagged path (= shift of 0 to the actual csum state)
            // we want to propagate the single extra unshifted byte
            if (!tr->next || !found.states_to_shift.count(tr->next->name)) {
                new_tr->shift = tr->shift - shift_amt / 8;
                LOG4("RemoveNegativeDeposits: Transition from "
                     << ps->name << " to " << (tr->next ? tr->next->name : "--END--")
                     << " changed: " << tr->shift << "->" << new_tr->shift);
            }
            new_tr->saves = *(new_tr->saves.apply(ShiftPacketRVal(shift_amt)));
            new_transitions.push_back(new_tr);
        }
        new_ps->transitions = new_transitions;
        return new_ps;
    }
};

/**
 * \defgroup LowerParserIR LowerParserIR
 * \ingroup LowerParser
 * \brief Generates a lowered version of the parser IR in this program and swaps
 *        it in for the existing representation.
 */
struct LowerParserIR : public PassManager {
    std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers;

    LowerParserIR(const PhvInfo &phv, BFN_MAYBE_UNUSED const FieldDefUse &defuse,
                  ClotInfo &clotInfo,
                  BFN_MAYBE_UNUSED const ParserHeaderSequences &parserHeaderSeqs,
                  std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers,
                  PhvLogging::CollectDefUseInfo *defuseInfo)
        : origParserZeroInitContainers(origParserZeroInitContainers) {
        auto *allocateParserChecksums = new AllocateParserChecksums(phv, clotInfo);
        auto *parser_info = new CollectParserInfo;
        auto *lower_parser_info = new CollectLoweredParserInfo;
        auto *find_negative_deposits = new FindNegativeDeposits;
        auto *computeLoweredParserIR = new ComputeLoweredParserIR(
            phv, clotInfo, *allocateParserChecksums, origParserZeroInitContainers);
        auto *replaceLoweredParserIR = new ReplaceParserIR(*computeLoweredParserIR);

        addPasses({LOGGING(4) ? new DumpParser("before_parser_lowering", false, true) : nullptr,
                   new SplitGreedyParserStates(),
                   LOGGING(4) ? new DumpParser("after_greedy_split", false, true) : nullptr,
                   new ResolveParserConstants(phv, clotInfo),
                   new ParserCopyProp(phv),
                   parser_info,
                   new SplitParserState(phv, clotInfo, *parser_info),
                   find_negative_deposits,
                   new RemoveNegativeDeposits(*find_negative_deposits),
                   new AllocateParserMatchRegisters(phv),
                   LOGGING(4) ? new DumpParser("before_elim_empty_states") : nullptr,
                   parser_info,
                   new EliminateEmptyStates(*parser_info),
                   allocateParserChecksums,
                   LOGGING(4) ? new DumpParser("after_alloc_parser_csums") : nullptr,
                   LOGGING(4) ? new DumpParser("final_hlir_parser") : nullptr,
                   static_cast<Visitor *>(computeLoweredParserIR),
                   static_cast<Visitor *>(replaceLoweredParserIR),
                   LOGGING(4) ? new DumpParser("after_parser_lowering") : nullptr,
                   new PassRepeated(
                       {lower_parser_info,
                        new HoistCommonMatchOperations(*lower_parser_info, *computeLoweredParserIR,
                                                       clotInfo),
                        LOGGING(4) ? new DumpParser("after_hoisting") : nullptr, lower_parser_info,
                        new MergeLoweredParserStates(*lower_parser_info, *computeLoweredParserIR,
                                                     clotInfo, defuseInfo),
                        LOGGING(4) ? new DumpParser("after_merging") : nullptr}),
                   LOGGING(4) ? new DumpParser("final_llir_parser") : nullptr});
    }
};

/**
 * @ingroup LowerDeparserIR
 * @brief Replace deparser IR with lowered version.
 *
 * Replace the high-level deparser IR version of each deparser with the lowered
 * version generated by ComputeLoweredDeparserIR.
 */
struct ReplaceDeparserIR : public DeparserTransform {
    ReplaceDeparserIR(const IR::BFN::LoweredDeparser *igLoweredDeparser,
                      const IR::BFN::LoweredDeparser *egLoweredDeparser)
        : igLoweredDeparser(igLoweredDeparser), egLoweredDeparser(egLoweredDeparser) {}

 private:
    const IR::BFN::AbstractDeparser *preorder(IR::BFN::Deparser *deparser) override {
        prune();
        return deparser->gress == INGRESS ? igLoweredDeparser : egLoweredDeparser;
    }

    const IR::BFN::LoweredDeparser *igLoweredDeparser;
    const IR::BFN::LoweredDeparser *egLoweredDeparser;
};

/**
 * \defgroup LowerDeparserIR LowerDeparserIR
 * \ingroup LowerParser
 *
 * \brief Replace deparser IR with lowered version that references containers instead of fields.
 *
 * Generate a lowered version of the parser IR in this program and swap it in
 * in place of the existing representation.
 *
 * The pass does this by:
 *  1. Replacing field/checksum emits that are covered by CLOTs with EmitClot objects.
 *  2. Generating lowered version of the deparser and swapping them in.
 *  3. Coalescing the learning digest to remove consecutive uses of the same container.
 */
struct LowerDeparserIR : public PassManager {
    LowerDeparserIR(const PhvInfo &phv, ClotInfo &clot) {
        auto *rewriteEmitClot = new RewriteEmitClot(phv, clot);
        auto *computeLoweredDeparserIR =
            new ComputeLoweredDeparserIR(phv, clot);  // NOLINT(whitespace/parens)
        addPasses({
            rewriteEmitClot,
            computeLoweredDeparserIR,
            new ReplaceDeparserIR(computeLoweredDeparserIR->igLoweredDeparser,
                                  computeLoweredDeparserIR->egLoweredDeparser),
            new CoalesceLearning,
        });
    }
};

/**
 * \ingroup LowerParser
 * \brief Issues warning for invalid container participating in ternary match.
 *
 * If a container that participates in ternary match is invalid, model(HW)
 * uses the last valid value in that container to perform the match.
 * To help user avoid matching on stale value in container, we issue warning
 * message so that user doesn't fall through this trapdoor.
 */
class WarnTernaryMatchFields : public MauInspector {
    const PhvInfo &phv;
    ordered_map<const IR::MAU::Table *, ordered_set<const PHV::Field *>> ternary_match_fields;

 public:
    explicit WarnTernaryMatchFields(const PhvInfo &phv) : phv(phv) {}

    bool preorder(const IR::MAU::Table *tbl) override {
        if (!tbl->layout.ternary) return false;

        auto &ixbar_use = tbl->resources->match_ixbar;

        for (auto &b : ixbar_use->use) {
            for (auto &fi : b.field_bytes) {
                auto f = phv.field(fi.field);
                ternary_match_fields[tbl].insert(f);
            }
        }

        // TODO check if user has already included the header validity bits
        // and if ternary match table is predicated by a header validty gateway
        // table so that we don't spew too many spurious warnings.

        return false;
    }

    void end_apply() override {
        for (auto &tf : ternary_match_fields) {
            for (auto f : tf.second) {
                if (!f->is_invalidate_from_arch()) continue;

                std::stringstream ss;

                ss << "Matching on " << f->name << " in table " << tf.first->name << " (implemented"
                   << " with ternary resources) before it has been assigned can result in "
                   << "non-deterministic lookup behavior."
                   << "\nConsider including in the match key an additional metadata"
                   << " field that indicates whether the field has been assigned.";

                warning("%1%", ss.str());
            }
        }
    }
};

}  // namespace Parde::Lowered

/**
 * \class LowerParser
 *
 * Sub-passes:
 *  - CharacterizeParser
 *  - CollectLoweredParserInfo
 *  - ComputeBufferRequirements
 *  - ComputeInitZeroContainers (Tofino 1 only)
 *  - ComputeMultiWriteContainers
 *  - LowerDeparserIR
 *  - LowerParserIR
 *  - PragmaNoInit
 *  - WarnTernaryMatchFields
 */
LowerParser::LowerParser(const PhvInfo &phv, ClotInfo &clot, const FieldDefUse &defuse,
                         const ParserHeaderSequences &parserHeaderSeqs,
                         PhvLogging::CollectDefUseInfo *defuseInfo)
    : Logging::PassManager("parser"_cs, Logging::Mode::AUTO) {
    using namespace Parde::Lowered;

    auto pragma_no_init = new PragmaNoInit(phv);
    auto compute_init_valid = new ComputeInitZeroContainers(
        phv, defuse, pragma_no_init->getFields(), origParserZeroInitContainers);
    auto parser_info = new CollectLoweredParserInfo;
    auto lower_parser_ir = new LowerParserIR(phv, defuse, clot, parserHeaderSeqs,
                                             origParserZeroInitContainers, defuseInfo);

    addPasses({pragma_no_init, lower_parser_ir, new LowerDeparserIR(phv, clot),
               new WarnTernaryMatchFields(phv),
               Device::currentDevice() == Device::TOFINO ? compute_init_valid : nullptr,
               parser_info, new ComputeMultiWriteContainers(phv, *parser_info),
               new ComputeBufferRequirements, new CharacterizeParser});
}
