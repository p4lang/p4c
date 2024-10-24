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

#include "allocate_clot.h"

#include <optional>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/parde/count_strided_header_refs.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/pragma/pa_alias.h"
#include "check_clot_groups.h"
#include "clot_candidate.h"
#include "field_pov_analysis.h"
#include "field_slice_extract_info.h"
#include "header_validity_analysis.h"
#include "lib/log_fixup.h"
#include "merge_desugared_varbit_valids.h"

/**
 * \ingroup parde
 * This implements a greedy CLOT-allocation algorithm, as described in
 * \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
 */
class GreedyClotAllocator : public Visitor {
    ClotInfo &clotInfo;
    const PhvInfo &phvInfo;
    const CollectParserInfo &parserInfo;
    Logging::FileLog *log = nullptr;
    const bool logAllocation;

 public:
    explicit GreedyClotAllocator(const PhvInfo &phvInfo, ClotInfo &clotInfo, bool logAllocation)
        : clotInfo(clotInfo),
          phvInfo(phvInfo),
          parserInfo(clotInfo.parserInfo),
          logAllocation(logAllocation) {}

 private:
    typedef std::map<const Pseudoheader *, ordered_map<const PHV::Field *, FieldSliceExtractInfo *>,
                     Pseudoheader::Less>
        FieldSliceExtractInfoMap;
    typedef std::set<const ClotCandidate *, ClotCandidate::Greater> ClotCandidateSet;

    /// Generates a FieldExtractInfo object for each field that is (1) extracted in the
    /// subgraph rooted at the given state and (2) can be part of a clot (@see
    /// ClotInfo::can_be_in_clot).
    ///
    /// @return The FieldExtractInfo object.
    ///
    /// This method assumes the graph is an unrolled DAG.
    ///
    /// @invariant @p state is not an element of  @p visited
    FieldExtractInfo *group_extracts(
        const IR::BFN::ParserGraph *graph, const IR::BFN::ParserState *state = nullptr,
        FieldExtractInfo *result = nullptr,
        ordered_set<std::pair<const IR::BFN::ParserState *, unsigned>> *visited = nullptr,
        unsigned stack_offset = 0) {
        // Initialize parameters if needed.
        if (!state) state = graph->root;
        if (!result) result = new FieldExtractInfo();
        if (!visited)
            visited = new ordered_set<std::pair<const IR::BFN::ParserState *, unsigned>>();

        LOG6("Finding extracts in state " << state->name << " (offset=" << stack_offset << ")");
        visited->insert(std::make_pair(state, stack_offset));

        // Find all packet-sourced extracts in the current state.
        cstring header_stack_name;
        std::set<unsigned> header_stack_indices;
        if (clotInfo.field_range_.count(state)) {
            for (const auto &[field, bitrange] : clotInfo.field_range_.at(state)) {
                const auto *adj_field = field;
                if (auto stack_index = get_element_index(field)) {
                    cstring name = field->name;
                    cstring field_name = cstring(name.findlast('.') + 1);
                    cstring header_name = field->header();
                    header_name = header_name.before(strrchr(header_name, '['));
                    cstring new_field_name = header_name + "["_cs +
                                             cstring::to_cstring(*stack_index + stack_offset) +
                                             "]." + field_name;
                    header_stack_name = header_name;
                    header_stack_indices.emplace(*stack_index + stack_offset);
                    adj_field = phvInfo.field(new_field_name);
                }

                int min_packet_offset = parserInfo.get_min_shift_amount(state) + bitrange.lo;
                int max_packet_offset = parserInfo.get_max_shift_amount(state) + bitrange.lo;

                // Add to result->fieldMap.
                result->updateFieldMap(adj_field, state, stack_offset,
                                       static_cast<unsigned>(bitrange.lo), min_packet_offset,
                                       max_packet_offset);

                if (!clotInfo.can_be_in_clot(adj_field)) continue;

                BUG_CHECK(clotInfo.field_to_pseudoheaders_.count(adj_field),
                          "Field %s determined to be CLOT-eligible, but is not extracted (no "
                          "pseudoheader information)",
                          adj_field->name);

                // Add to result->pseudoheaderMap.
                for (auto pseudoheader : clotInfo.field_to_pseudoheaders_.at(adj_field)) {
                    result->updatePseudoheaderMap(pseudoheader, adj_field, state, stack_offset,
                                                  static_cast<unsigned>(bitrange.lo),
                                                  min_packet_offset, max_packet_offset);
                }
            }
        }
        if (header_stack_indices.size()) {
            LOG6("Indices for " << header_stack_name << ": " << header_stack_indices);
            result->updateHeaderStackMap(header_stack_name, state, header_stack_indices);
        }

        // Check the number of stack elements in the current state
        CountStridedHeaderRefs count;
        state->statements.apply(count);

        unsigned succ_offset = 0;
        if (state->stride && count.header_stack_to_indices.size() == 1) {
            // Case of > 1 is handled in compute_lowered_parser_ir
            auto &indices = count.header_stack_to_indices.begin()->second;
            succ_offset = stack_offset + indices.size();
        }

        // Recurse with the unvisited successors of the current state, if any.
        if (graph->successors().count(state)) {
            for (auto succ : graph->successors().at(state)) {
                if (visited->count(std::make_pair(succ, succ_offset))) continue;

                LOG6("Recursing with transition " << state->name << " -> " << succ->name);
                group_extracts(graph, succ, result, visited, succ_offset);
            }
        }

        return result;
    }

    /// Creates a CLOT candidate out of the given extracts, if possible, and adds the result to the
    /// given candidate set.
    ///
    /// Precondition: the extracts are a valid CLOT prefix.
    void try_add_clot_candidate(ClotCandidateSet *candidates, const Pseudoheader *pseudoheader,
                                std::vector<const FieldSliceExtractInfo *> &extracts) const {
        // Trim the first extract so that it extends past minimum CLOT position
        // (after port metadata) and is byte-aligned.
        extracts[0] = extracts.front()->trim_head_to_min_clot_pos();
        extracts[0] = extracts.front()->trim_head_to_byte();

        if (pseudoheader) {
            // Remove extracts until we find one that can end a CLOT.
            while (!extracts.empty() && !clotInfo.can_end_clot(extracts.back()))
                extracts.pop_back();
        } else {
            // For multiheader CLOTs, they share the same first slice in every parser state, but
            // all other slices could be different. Therefore, we need to make sure that, after we
            // remove extracts in each state, the extract sequence from the deparser remains
            // contiguous.
            std::map<const IR::BFN::ParserState *, std::vector<const FieldSliceExtractInfo *>>
                state_to_extracts;
            for (const auto &state : extracts.front()->states()) {
                std::vector<const FieldSliceExtractInfo *> extracts_copy(extracts);

                while (!extracts_copy.empty() && (!clotInfo.can_end_clot(extracts_copy.back()) ||
                                                  !extracts_copy.back()->states().count(state)))
                    extracts_copy.pop_back();

                // If no extracts are left in any of the states, forming a CLOT with these
                // extracts is impossible. Therefore, do not create a ClotCandidate.
                if (extracts_copy.empty()) return;

                state_to_extracts[state] = extracts_copy;
            }
            for (auto it = std::next(extracts.begin()); it != extracts.end();) {
                bool any_extracts_contains_it = false;
                for (const auto &kv : state_to_extracts) {
                    if (std::find(kv.second.begin(), kv.second.end(), *it) != kv.second.end()) {
                        any_extracts_contains_it = true;
                        break;
                    }
                }
                if (any_extracts_contains_it)
                    ++it;
                else
                    it = extracts.erase(it);
            }
        }

        // If we still have extracts, create a CLOT candidate out of those.
        if (!extracts.empty()) {
            // Trim the last extract so that it is byte-aligned.
            extracts.back() = extracts.back()->trim_tail_to_byte();

            // Further trim the last extract so that it does not extend past the maximum CLOT
            // position.
            extracts.back()->trim_tail_to_max_clot_pos();

            for (const auto &existing_candidate : *candidates) {
                if (existing_candidate->extracts().size() == extracts.size() &&
                    std::equal(existing_candidate->extracts().begin(),
                               existing_candidate->extracts().end(), extracts.begin(),
                               [](const FieldSliceExtractInfo *a, const FieldSliceExtractInfo *b) {
                                   return *a == *b;
                               })) {
                    LOG4("Tried to add a candidate with the same extracts "
                         << "as an existing candidate.");
                    return;
                }
            }

            const ClotCandidate *candidate = new ClotCandidate(clotInfo, pseudoheader, extracts);
            candidates->insert(candidate);
            LOG6("  Created candidate");
            LOG6(candidate->print());
        }
    }

    /// A helper. Adds to the given @p result any CLOT candidates that can be made from a given
    /// set of extracts into field slices that all belong to a given pseudoheader.
    void add_clot_candidates(
        ClotCandidateSet *result, const Pseudoheader *pseudoheader,
        const std::map<const PHV::Field *, std::vector<const FieldSliceExtractInfo *>> &extract_map)
        const {
        // Invariant: `extracts` forms a valid prefix for a potential CLOT candidate. When
        // `extracts` is non-empty, `next_offset_by_state` maps each parser state for the potential
        // CLOT candidate to the expected state-relative offset for the next field slice. When
        // `extracts` is empty, then so is `next_offset_by_state`.
        std::vector<const FieldSliceExtractInfo *> extracts;
        ordered_map<const IR::BFN::ParserState *, unsigned> next_offset_by_state;

        LOG6("Finding CLOT candidates for pseudoheader " << pseudoheader->id);
        for (auto field : pseudoheader->fields) {
            LOG6("  Considering field " << field->name);

            if (!extract_map.count(field)) {
                if (extracts.empty()) {
                    LOG6("  Can't start CLOT with " << field->name << ": not extracted");
                } else {
                    // We have a break in contiguity. Create a new CLOT candidate, if possible, and
                    // set things up for the next candidate.
                    LOG6("  Contiguity break");
                    try_add_clot_candidate(result, pseudoheader, extracts);
                    extracts.clear();
                    next_offset_by_state.clear();
                }

                continue;
            }

            for (auto extract_info : extract_map.at(field)) {
                if (!extracts.empty()) {
                    // We have a break in contiguity if the current extract_info is extracted in a
                    // different set of states than the current candidate or if the current
                    // extract_info has any state-relative bit offsets that are different from what
                    // we are expecting.
                    auto &extract_state_bit_offsets = extract_info->state_bit_offsets();

                    bool have_contiguity_break =
                        extract_state_bit_offsets.size() != next_offset_by_state.size();
                    if (have_contiguity_break) {
                        LOG6("  Contiguity break: extracted in a different set of states");
                    } else {
                        for (auto &entry : next_offset_by_state) {
                            auto &state = entry.first;
                            auto next_offset = entry.second;

                            have_contiguity_break = !extract_state_bit_offsets.count(state);
                            if (have_contiguity_break) {
                                LOG6("  Contiguity break: extracted in a different set of states");
                                break;
                            }

                            have_contiguity_break =
                                extract_state_bit_offsets.at(state) != next_offset;
                            if (have_contiguity_break) {
                                LOG6("  Contiguity break in state " << state->name);
                                break;
                            }
                        }
                    }

                    if (have_contiguity_break) {
                        // We have a break in contiguity. Create a new CLOT candidate, if possible,
                        // and set things up for the next candidate.
                        try_add_clot_candidate(result, pseudoheader, extracts);
                        extracts.clear();
                        next_offset_by_state.clear();
                    }
                }

                if (extracts.empty()) {
                    // Starting a new candidate.
                    if (!clotInfo.can_start_clot(extract_info)) continue;

                    auto &state_bit_offsets = extract_info->state_bit_offsets();
                    next_offset_by_state.insert(state_bit_offsets.begin(), state_bit_offsets.end());
                }

                // Add to the existing prefix.
                extracts.push_back(extract_info);
                auto slice = extract_info->slice();
                for (auto &next_offset : Values(next_offset_by_state)) {
                    next_offset += slice->size();
                }

                LOG6("  Added " << slice->shortString() << " to CLOT candidate prefix");
            }
        }

        // If possible, create a new CLOT candidate from the remaining extracts.
        if (!extracts.empty()) try_add_clot_candidate(result, pseudoheader, extracts);
    }

    // Precondition: all FieldSliceExtractInfo instances in the given map correspond to fields that
    // satisfy @ref ClotInfo::can_be_in_clot.
    //
    // This method's responsibility, then, is to ensure the following for each candidate:
    //   - A set of contiguous bits is extracted from the packet.
    //   - The aggregate set of extracted bits is byte-aligned in the packet.
    //   - All extracted field slices are contiguous.
    //   - No pair of extracted field slices have a deparserNoPack constraint.
    //   - Neither the first nor last field in the candidate is modified or is a checksum.
    //   - The fields in the candidate are all extracted in the same set of parser states.
    ClotCandidateSet *find_clot_candidates(
        const FieldExtractInfo::PseudoheaderMap &extract_info_map) {
        auto result = new ClotCandidateSet();

        for (const auto &entry : extract_info_map) {
            std::map<const PHV::Field *, std::vector<const FieldSliceExtractInfo *>> submap;
            for (const auto &subentry : entry.second) {
                submap[subentry.first].push_back(subentry.second);
            }

            add_clot_candidates(result, entry.first, submap);
        }

        return result;
    }

    /// Helper to determine whether an adjustment is needed to create an inter-CLOT gap between two
    /// CLOT candidates, when @p c1 is parsed before @p c2. If @p c1 is never parsed before @p c2,
    /// or if @p c1 and @p c2 are always separated by at least the inter-CLOT gap requirement,
    /// then this conservatively returns true.
    bool needInterClotGap(const ClotCandidate *c1, const ClotCandidate *c2,
                          const HeaderValidityAnalysis::ResultMap header_removals) const {
        BUG_CHECK(c1->thread() == c2->thread(),
                  "Candidate %1% comes from %2%, but candidate %3% comes from %4%", c1->id,
                  c1->thread(), c2->id, c2->thread());

        auto gress = c1->thread();
        const auto &deparse_graph = clotInfo.deparse_graph_[gress];

        auto gaps = c1->byte_gaps(parserInfo, c2);
        if (gaps.empty()) {
            LOG5("    Candidate " << c1->id << " is never parsed before candidate " << c2->id);
            return (c1->pseudoheader != c2->pseudoheader) ? true : false;
        }

        LOG5("    Candidate " << c1->id << " might be parsed before candidate " << c2->id);

        // Need gap if CLOTs are never zero-separated.
        if (!gaps.count(0)) {
            LOG5("      Candidate " << c1->id << " never appears immediately before candidate "
                                    << c2->id << " in parsed packet");
            return true;
        }

        // Need gap if separation between CLOTs can be non-zero and smaller than the required
        // inter-CLOT gap.
        for (unsigned i = 1; i < Device::pardeSpec().byteInterClotGap(); i++) {
            if (gaps.count(i)) {
                LOG5("      Candidates might be separated by " << i << " bytes in parsed packet");
                return true;
            }
        }

        // Need gap if c1 can become invalid in MAU when c2 is still valid.
        auto c1_pov_bits = c1->pov_bits;
        auto c2_pov_bits = c2->pov_bits;
        auto check = [&](const PHV::FieldSlice *f1) {
            auto check = [&](const PHV::FieldSlice *f2) {
                return f1 != f2 && header_removals.at({f1, f2}).count({f1});
            };
            return std::any_of(c2_pov_bits.begin(), c2_pov_bits.end(), check);
        };
        if (std::any_of(c1_pov_bits.begin(), c1_pov_bits.end(), check)) {
            LOG5("      Candidate " << c1->id << " might be invalidated by MAU while candidate "
                                    << c2->id << " remains valid");
            return true;
        }

        // Need gap if c2 can be deparsed before c1.
        const auto *c2_last_field = c2->extracts().back()->slice()->field();
        const auto *c1_first_field = c1->extracts().front()->slice()->field();
        if (deparse_graph.canReach(c2_last_field, c1_first_field)) {
            LOG5("      Candidate " << c2->id << " might be deparsed before candidate " << c1->id);
            return true;
        }

        // Need gap if c1 can become separated from c2 during deparsing.
        {
            const auto *c1_last_slice = c1->extracts().back()->slice();
            const auto *c2_first_slice = c2->extracts().front()->slice();
            const auto *c1_last_field = c1_last_slice->field();
            const auto *c2_first_field = c2_first_slice->field();

            if (c1_last_field == c2_first_field) {
                // c1's last slice and c2's first slice come from the same field. This means that
                // the "gaps" set should be a singleton containing the exact separation between the
                // two slices. We already know that 0 ∈ gaps, so expect that |gaps| = 1.
                if (gaps.size() != 1) {
                    std::stringstream msg;
                    msg << "Unexpectedly got more than one gap size between two slices of the "
                        << "same field. Slices " << c1_last_slice->shortString() << " and "
                        << c2_first_slice->shortString() << " have gaps {";

                    bool first = true;
                    for (auto gap : Keys(gaps)) {
                        if (!first) msg << ", ";
                        msg << gap;
                        first = false;
                    }

                    msg << "}";

                    BUG("%s", msg.str());
                }
            } else {
                // c1's last slice and c2's first slice come from different fields. Because we know
                // 0 ∈ gaps, c1's last slice must cover the last bit of its field, and c2's first
                // slice must cover the first bit of its field. It suffices, then, to check
                // entities that might be deparsed between those two fields.
                for (const auto &nodeInfo :
                     deparse_graph.nodesBetween(c1_last_field, c2_first_field)) {
                    if (nodeInfo.isConstant()) {
                        LOG5("      Constant " << nodeInfo.constant
                                               << " might be deparsed "
                                                  "between CLOT candidates");
                        return true;
                    }

                    const auto *f = nodeInfo.field;

                    // If a state exists in the parser where the field f has its POV bit set
                    // but f is not extracted from the packet (zero init or extracted from
                    // constant), then the states where that happens must not be in the same paths
                    // through the parser as the states containing the two ClotCandidates. If they
                    // are, then the candidates could be adjacent in the parser but not adjacent in
                    // the deparser, which is not allowed. Thus, a gap would be required.
                    if (!clotInfo.extracted_with_pov(f)) {
                        std::set<const IR::BFN::ParserState *>
                            states_where_f_is_not_extracted_from_packet;
                        for (const auto &[state, source] : clotInfo.field_to_parser_states_.at(f)) {
                            if (!source->is<IR::BFN::PacketRVal>()) {
                                states_where_f_is_not_extracted_from_packet.insert(state);
                            }
                        }
                        for (const auto &f_state : states_where_f_is_not_extracted_from_packet) {
                            for (const auto &[c1_state, c2_state] : gaps.at(0)) {
                                const auto &parser_graph = parserInfo.graph(f_state);
                                if (!parser_graph.is_ancestor(f_state, c1_state) &&
                                    (!parser_graph.is_ancestor(c1_state, f_state) ||
                                     !parser_graph.is_ancestor(f_state, c2_state)) &&
                                    !parser_graph.is_ancestor(c2_state, f_state))
                                    continue;
                                LOG5("      Field " << f->name
                                                    << " might be deparsed between candidates, "
                                                       "and not be sourced from the packet");
                                return true;
                            }
                        }
                    }

                    // The field f might be deparsed between c1 and c2, so the two CLOT candidates
                    // might be separated if f's header might be added by the MAU pipeline.
                    if (clotInfo.is_added_by_mau(f->header())) {
                        LOG5("      Field " << f->name
                                            << " might be deparsed between candidates, "
                                               "and its header might be added by MAU");
                        return true;
                    }

                    // The two CLOT candidates might be separated if there is a path through the
                    // parser in which c1 is parsed immediately before c2, and f is also parsed.
                    //
                    // fei->fieldMap only contains an entry for f if f is parsed from packet data,
                    // not if it is parsed from a constant. So look directly at the
                    // field_to_parser_states_ map as that contains packet and constant extracts.
                    if (clotInfo.field_to_parser_states_.count(f)) {
                        for (const auto *fState : Keys(clotInfo.field_to_parser_states_.at(f))) {
                            for (const auto &[c1State, c2State] : gaps.at(0)) {
                                // Look at cases where f comes before c1 or after c2, or at the same
                                // time as either.
                                // (c1 and c2 are adjacent, so we can't have a case of f after c1
                                // without also being after c2 and vice versa)
                                if (fState == c1State || fState == c2State ||
                                    parserInfo.graph(fState).is_ancestor(fState, c1State) ||
                                    parserInfo.graph(fState).is_ancestor(c2State, fState)) {
                                    LOG5("      Field " << f->name
                                                        << " might be inserted between candidates");
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    /// Adjusts a CLOT candidate to account for the allocation of another (possibly the same)
    /// candidate.
    ClotCandidateSet *adjust_for_allocation(
        const ClotCandidate *to_adjust, const ClotCandidate *allocated,
        const HeaderValidityAnalysis::ResultMap header_removals) const {
        const auto GAP_BYTES = Device::pardeSpec().byteInterClotGap();
        const auto GAP_BITS = 8 * GAP_BYTES;

        LOG5("");
        LOG5("  Adjusting candidate " << to_adjust->id << " for allocated CLOT");

        ClotCandidateSet *result = new ClotCandidateSet();

        // If the states from one candidate are mutually exclusive with the states from another,
        // then no need to adjust.
        const auto to_adjust_states = to_adjust->states();
        const auto allocated_states = allocated->states();
        const auto &graph = parserInfo.graph(parserInfo.parser(*allocated_states.begin()));
        bool mutually_exclusive = true;
        for (auto &to_adjust_state : to_adjust_states) {
            for (auto &allocated_state : allocated_states) {
                mutually_exclusive = graph.is_mutex(to_adjust_state, allocated_state);
                if (!mutually_exclusive) break;
            }
            if (!mutually_exclusive) break;
        }

        if (mutually_exclusive) {
            LOG5("    No need to adjust: states are mutually exclusive");
            result->insert(to_adjust);
            return result;
        }

        // Determine the inter-CLOT gap sizes needed before and after the allocated candidate.
        bool allocatedNeedsPreGap = needInterClotGap(to_adjust, allocated, header_removals);
        int preGapBits = allocatedNeedsPreGap ? GAP_BITS : 0;

        bool allocatedNeedsPostGap = needInterClotGap(allocated, to_adjust, header_removals);
        int postGapBits = allocatedNeedsPostGap ? GAP_BITS : 0;

        // If the candidate occurs immediately after a CLOT that has already been allocated, and
        // the just-allocated CLOT conflicts with the first byte of the candidate, then we need to
        // remove the first few bytes to create a sufficient inter-CLOT gap with the previously
        // allocated CLOT.
        //
        // Given this constraint, this le_bitinterval tracks which bits in the candidate can be
        // included in the adjusted CLOT candidate. This is kept in low-endian format to make
        // manipulations with field slices easier later on.
        le_bitinterval candidate_interval = StartLen(0, to_adjust->max_size_in_bits());
        if (to_adjust->afterAllocatedClot) {
            // Get the first extract and figure out what parts of it don't conflict.
            const auto *first_extract = to_adjust->extracts().front();
            const auto *non_conflicts =
                first_extract->remove_conflicts(parserInfo, preGapBits, allocated, postGapBits);

            // Figure out if first byte conflicts.
            bool first_byte_conflicts;
            if (non_conflicts->empty()) {
                first_byte_conflicts = true;
            } else {
                const auto *first_adjusted = non_conflicts->front();
                first_byte_conflicts =
                    first_extract->slice()->range().hi != first_adjusted->slice()->range().hi;
            }

            // Adjust candidate_interval as needed.
            if (first_byte_conflicts) {
                LOG5("    First byte of candidate " << to_adjust->id << " conflicts");
                int new_size = std::max(0L, candidate_interval.size() - GAP_BITS);
                candidate_interval = candidate_interval.resizedToBits(new_size);
            }
        }

        // Similarly, we may need to remove the last few bytes of the candidate.
        if (to_adjust->beforeAllocatedClot) {
            // Get the last extract and figure out what parts of it don't conflict.
            const auto *last_extract = to_adjust->extracts().back();
            const auto *non_conflicts =
                last_extract->remove_conflicts(parserInfo, preGapBits, allocated, postGapBits);

            // Figure out if last byte conflicts.
            bool last_byte_conflicts;
            if (non_conflicts->empty()) {
                last_byte_conflicts = true;
            } else {
                const auto *last_adjusted = non_conflicts->back();
                last_byte_conflicts =
                    last_extract->slice()->range().lo != last_adjusted->slice()->range().lo;
            }

            // Adjust candidate_interval as needed.
            if (last_byte_conflicts) {
                LOG5("    Last byte of candidate " << to_adjust->id << " conflicts");
                int new_size = std::max(0L, candidate_interval.size() - GAP_BITS);
                candidate_interval =
                    candidate_interval.resizedToBits(new_size).shiftedByBits(GAP_BITS);
            }
        }

        // Figure out whether the candidates conflict. Build an extract_map for the set of
        // non-conflicting extracts. This map will be used for the slow path below.
        bool have_conflict = false;
        std::map<const PHV::Field *, std::vector<const FieldSliceExtractInfo *>> extract_map;
        unsigned extract_bit_pos = to_adjust->max_size_in_bits();
        for (auto extract : to_adjust->extracts()) {
            const PHV::FieldSlice *slice = extract->slice();
            extract_bit_pos -= slice->size();

            // Adjust the extract so it lands within candidate_bitinterval. If the extract is
            // disjoint from the interval, then use nullptr to represent the empty extract.
            auto trim_range = toHalfOpenRange(slice->range())
                                  .shiftedByBits(extract_bit_pos)
                                  .intersectWith(candidate_interval)
                                  .shiftedByBits(-extract_bit_pos)
                                  .shiftedByBits(-slice->range().lo);
            const FieldSliceExtractInfo *adjusted_extract =
                trim_range.empty() ? nullptr : extract->trim(trim_range.lo, trim_range.size());

            const std::vector<const FieldSliceExtractInfo *> *non_conflicts =
                adjusted_extract ? adjusted_extract->remove_conflicts(parserInfo, preGapBits,
                                                                      allocated, postGapBits)
                                 : new std::vector<const FieldSliceExtractInfo *>();

            // Figure out whether this extract conflicts with the allocated candidate, and add the
            // non-conflicting extracts to extract_map.
            bool extract_conflicts = false;
            if (non_conflicts->empty()) {
                extract_conflicts = true;
                LOG5("    Removed " << slice->shortString());
            }
            for (auto adjusted : *non_conflicts) {
                extract_conflicts |= extract != adjusted;
                extract_map[slice->field()].push_back(adjusted);
            }

            have_conflict |= extract_conflicts;
            if (LOGGING(5) && extract_conflicts && !non_conflicts->empty()) {
                LOG5("    Replaced " << slice->shortString());

                bool first_replacement = true;
                for (auto adjusted : *non_conflicts) {
                    LOG5("       " << (first_replacement ? "with: " : "      ")
                                   << adjusted->slice()->shortString());
                    first_replacement = false;
                }
            }
        }

        // Fast path, taken when the candidates don't conflict:
        //   Return a singleton containing the candidate that we are adjusting, after updating the
        //   candidate with information about whether it appears immediately before or after an
        //   allocated CLOT.
        //
        // Slow path, taken when the candidates conflict:
        //   Delegate to add_clot_candidates.
        if (!have_conflict) {
            if (LOGGING(5)) {
                LOG5("    Candidate does not conflict with allocated CLOT");

                if (!allocatedNeedsPostGap) {
                    if (to_adjust->afterAllocatedClot) {
                        LOG5(
                            "    Candidate already appears after an allocated CLOT with 0-byte "
                            "gap");
                    } else {
                        LOG5(
                            "    Marking candidate as appearing after allocated CLOT with 0-byte "
                            "gap");
                    }
                }

                if (!allocatedNeedsPreGap) {
                    if (to_adjust->beforeAllocatedClot) {
                        LOG5(
                            "    Candidate already appears before an allocated CLOT with 0-byte "
                            "gap");
                    } else {
                        LOG5(
                            "    Marking candidate as appearing before allocated CLOT with "
                            "0-byte gap");
                    }
                }
            }

            result->insert(
                to_adjust->mark_adjacencies(!allocatedNeedsPostGap, !allocatedNeedsPreGap));
        } else if (!extract_map.empty() && to_adjust->pseudoheader) {
            add_clot_candidates(result, to_adjust->pseudoheader, extract_map);
        } else if (!to_adjust->pseudoheader) {
            result->erase(to_adjust);
        }

        return result;
    }

    /// Resizes a CLOT candidate so that it fits into the maximum CLOT length.
    ///
    /// @return the resized CLOT candidate, or nullptr if resizing fails.
    //
    // We can be fancier here, but this just greedily picks the first longest subsequence of
    // extracts that fits the CLOT-allocation constraints.
    const ClotCandidate *resize(const ClotCandidate *candidate) const {
        const int MAX_SIZE = Device::pardeSpec().byteMaxClotSize() * 8;
        unsigned best_start_idx = 0;
        unsigned best_end_idx = 0;
        int best_size = 0;
        bool need_resize = false;

        // Since the CLOT candidate looks the same (has the same contiguous sequence of field
        // slices) in all parser states that it appears in, we can just work within a single such
        // parser state.
        const auto *state = *Keys(candidate->state_bit_offsets()).begin();

        const auto &extracts = candidate->extracts();
        for (auto start_idx : candidate->can_start_indices()) {
            auto start = extracts.at(start_idx)->trim_head_to_min_clot_pos();
            start = start->trim_head_to_byte();

            // Find the rightmost end_idx that fits in the maximum CLOT size.
            //
            // We do this by taking start_bit_offset to be that of the last byte (or fraction of a
            // byte) in `start`, and finding the rightmost index whose offset lands within the
            // minimum CLOT size of start_bit_offset.

            int start_bit_offset =
                (start->state_bit_offset(state) + start->slice()->size() - 1) / 8 * 8;

            for (auto end_idx : candidate->can_end_indices()) {
                if (start_idx > end_idx) break;

                auto end = extracts.at(end_idx)->trim_tail_to_byte();
                int end_bit_offset = end->state_bit_offset(state);
                if (end_bit_offset - start_bit_offset >= MAX_SIZE) continue;

                int full_size =
                    end_bit_offset + end->slice()->size() - start->state_bit_offset(state);

                int cur_size = std::min(MAX_SIZE, full_size);

                if (cur_size > best_size) {
                    best_start_idx = start_idx;
                    best_end_idx = end_idx;
                    best_size = cur_size;
                    need_resize = cur_size != full_size;
                }

                break;
            }

            if (best_size == MAX_SIZE) break;
        }

        if (best_size == 0) return nullptr;
        if (best_start_idx == 0 && best_end_idx == extracts.size() - 1 && !need_resize)
            return candidate;

        // Create the list of extracts for the resized candidate.
        auto resized = new std::vector<const FieldSliceExtractInfo *>();
        for (auto idx = best_start_idx; idx <= best_end_idx; idx++)
            resized->push_back(extracts.at(idx));

        // Trim first and last extract to byte boundary.
        auto start = resized->front() = resized->front()->trim_head_to_byte();
        auto end = resized->back() = resized->back()->trim_tail_to_byte();

        // Trim last extract if we exceed the maximum CLOT length.
        int size =
            end->state_bit_offset(state) + end->slice()->size() - start->state_bit_offset(state);
        if (size > MAX_SIZE) {
            int trim_amount = size - MAX_SIZE;

            // Make sure we leave at least one bit in the extract, while maintaining byte
            // alignment.
            int end_size = end->slice()->size();
            if (end_size <= trim_amount) trim_amount = (end_size - 1) / 8 * 8;

            int new_end_start = trim_amount;
            int new_end_size = end_size - new_end_start;
            resized->back() = end = end->trim(new_end_start, new_end_size);
            size -= trim_amount;
        }

        // Trim the first extract if we still exceed the maximum CLOT length.
        if (size > MAX_SIZE) {
            int trim_amount = size - MAX_SIZE;
            int start_size = start->slice()->size();
            BUG_CHECK(start_size > trim_amount,
                      "Extract for %s is only %d bits, but need to trim %d bits to fit in CLOT",
                      start->slice()->shortString(), start_size, trim_amount);

            int new_start_size = start_size - trim_amount;
            resized->front() = start->trim(0, new_start_size);
        }

        auto result = new ClotCandidate(clotInfo, candidate->pseudoheader, *resized);
        LOG3("Resized candidate " << candidate->id << ":");
        LOG3(result->print());
        return result;
    }

    /// Get the element index for a field
    std::optional<unsigned> get_element_index(const PHV::Field *field) {
        cstring header_name = field->header();
        const char *index_pos = strrchr(header_name, '[');
        if (index_pos) {
            size_t idx_start = index_pos - header_name.c_str() + 1;
            size_t idx_len = header_name.size() - idx_start - 1;
            cstring header_inst = header_name.substr(idx_start, idx_len);
            return static_cast<unsigned>(std::atoi(header_inst.c_str()));
        }
        return std::nullopt;
    }

    /// Get the element index for an extract to a header stack element
    std::optional<unsigned> get_element_index(const FieldSliceExtractInfo *extract) {
        const auto *slice = extract->slice();
        return get_element_index(slice->field());
    }

    /// Identify which candidates correspond to header stacks extracted in loop states
    /// and record information about the stacks. Trim the list of candidates based on
    /// which states are missing elements.
    ClotCandidateSet *identify_stacks_and_trim_candidates(
        ClotCandidateSet *candidates,
        std::map<const ClotCandidate *, const IR::HeaderStack *> &stack_candidates,
        std::map<const IR::HeaderStack *, int> &stack_delta) {
        std::map<const IR::HeaderStack *, std::set<const IR::BFN::ParserState *>> stack_states;
        std::map<std::pair<const IR::BFN::ParserState *, const IR::HeaderStack *>,
                 std::set<unsigned>>
            stack_elements;

        auto &parser_state_header_stacks = clotInfo.parser_state_to_header_stacks();
        for (auto *candidate : *candidates) {
            bool is_stack = false;
            bool is_non_stack = false;
            const IR::HeaderStack *hs = nullptr;
            for (auto extract : candidate->extracts()) {
                for (const auto *state : extract->states()) {
                    if (parser_state_header_stacks.count(state)) {
                        is_stack = true;
                        hs = parser_state_header_stacks.at(state).front();
                        stack_states[hs].emplace(state);
                        auto state_stack = std::make_pair(state, hs);

                        // Identify the "delta": the number of elements processed
                        // by each state. If we have different numbers processed
                        // in different states, treat the delta as 1 (i.e., all elements
                        // of the stack will be processed).
                        if (clotInfo.header_stack_elements().count(state_stack)) {
                            int elems = clotInfo.header_stack_elements().at(state_stack).size();
                            if (!stack_delta.count(hs))
                                stack_delta.emplace(hs, elems);
                            else if (elems == stack_delta.at(hs))
                                stack_delta.emplace(hs, 1);
                        }

                        // Record the element being written
                        auto index = get_element_index(extract);
                        if (index) stack_elements[state_stack].emplace(*index);
                    } else {
                        is_non_stack = true;
                    }
                }
            }
            BUG_CHECK(!(is_stack && is_non_stack),
                      "Candidate %1% corresponds to a stack and non-stack extract?", candidate->id);
            if (is_stack) stack_candidates.emplace(candidate, hs);
        }

        // For the header stacks identified above, verify that candidates:
        //  - cover all the states that extract the stack elements
        //  - extract all elements of the stack
        // If either of these are not satisfied then none of the elements can be
        // extrcted into clots because we can't conditionally extract to CLOTs vs PHVs based
        // on the value of the parser offset counter.
        //
        // FIXME: We can extract to a mix of CLOTs and PHVs if all paths through the loop extract
        // identical numbers of CLOTs at identical stack indices.
        std::set<const IR::HeaderStack *> stacks_missing_elements;
        for (auto &[hs, states] : stack_states) {
            bool seen_all = true;
            for (auto &[other_state, stacks] : clotInfo.parser_state_to_header_stacks()) {
                for (auto *other_hs : stacks) {
                    if (other_hs == hs) {
                        auto state_stack = std::make_pair(other_state, hs);
                        seen_all &= states.count(other_state) &&
                                    clotInfo.header_stack_elements().at(state_stack) ==
                                        stack_elements.at(state_stack);
                        break;
                    }
                }
                if (!seen_all) break;
            }
            if (!seen_all) {
                LOG6("Missing stack elements for " << hs->name);
                stacks_missing_elements.emplace(hs);
            }
        }

        // Eliminate candidates corresponding to missing states
        auto *new_candidates = new ClotCandidateSet();
        for (auto *candidate : *candidates) {
            if (stack_candidates.count(candidate)) {
                if (!stacks_missing_elements.count(stack_candidates.at(candidate)))
                    new_candidates->insert(candidate);
                else
                    LOG4("Discarding candidate " << candidate->id
                                                 << " due to missing stack element");
            } else {
                new_candidates->insert(candidate);
            }
        }

        return new_candidates;
    }

    /// Uses a greedy algorithm to allocate the given candidates.
    void allocate(ClotCandidateSet *candidates,
                  const HeaderValidityAnalysis::ResultMap header_removals) {
        const auto MAX_CLOTS_PER_GRESS = Device::pardeSpec().numClotsPerGress();

        std::map<const ClotCandidate *, const IR::HeaderStack *> stack_candidates;
        std::map<const IR::HeaderStack *, int> stack_delta;

        candidates = identify_stacks_and_trim_candidates(candidates, stack_candidates, stack_delta);

        // Pool of available tags
        std::set<unsigned> free_tags;
        ;
        for (unsigned i = 0; i < MAX_CLOTS_PER_GRESS; ++i) free_tags.insert(free_tags.end(), i);

        // Invariant: all members of the candidate set can be allocated. That is, if we were to
        // allocate any single member, it would not violate any CLOT-allocation constraints.
        while (!candidates->empty()) {
            auto candidate = *(candidates->begin());

            // Resize the candidate before allocating.
            // Multiheader candidates are not resized.
            if (candidate->pseudoheader != nullptr) {
                auto resized = resize(candidate);
                if (!resized) {
                    // Resizing failed.
                    LOG3("Couldn't fit candidate " << candidate->id << " into a CLOT");
                    candidates->erase(candidate);
                    continue;
                }

                candidate = resized;
            }

            // Allocate the candidate.
            BUG_CHECK(candidate->bit_in_byte_offset() == 0,
                      "CLOT candidate is not byte-aligned\n%s", candidate->print());
            auto states = candidate->states();
            auto gress = candidate->thread();

            const auto *hs =
                stack_candidates.count(candidate) ? stack_candidates.at(candidate) : nullptr;
            int depth = hs ? hs->size : 1;
            int delta = hs && stack_delta.count(hs) ? stack_delta.at(hs) : 1;

            unsigned first_idx = 0;
            if (hs) {
                auto *extract = candidate->extracts().front();
                first_idx = *get_element_index(extract);
            }

            // Check whether sufficient tags exist at the required spacing if multiple elements need
            // allocating and identify the first tag in the sequence
            bool found = false;
            unsigned tag = 0;
            for (unsigned candidate : free_tags) {
                int next_tag = candidate + delta;
                bool miss = false;
                while (next_tag - candidate < static_cast<unsigned>(depth) - first_idx && !miss) {
                    miss = !free_tags.count(next_tag);
                    if (!miss) next_tag += delta;
                }
                if (!miss) {
                    found = true;
                    tag = candidate;
                    break;
                }
            }

            unsigned first_tag = tag;
            while (found && tag - first_tag < static_cast<unsigned>(depth) - first_idx) {
                auto clot = new Clot(gress);
                clot->tag = tag;
                if (LOGGING(3)) {
                    std::string extra = depth > 1
                                            ? " (" + std::to_string(first_idx + tag - first_tag) +
                                                  " of " + std::to_string(depth) +
                                                  ", step=" + std::to_string(delta) + ")"
                                            : "";
                    LOG3("Allocating CLOT " << clot->tag << " to candidate " << candidate->id
                                            << extra);

                    bool first_state = true;
                    for (auto *state : states) {
                        LOG3("  " << (first_state ? "states: " : "        ") << state->name);
                        first_state = false;
                    }
                }
                clotInfo.add_clot(clot, states);

                // Add field slices.
                for (auto extract : candidate->extracts()) {
                    auto extract_slice = extract->slice();
                    const PHV::FieldSlice *slice = extract_slice;
                    if (tag != first_tag) {
                        cstring name = slice->field()->name;
                        cstring field_name = cstring(name.findlast('.') + 1);
                        cstring new_field_name = hs->name + "["_cs +
                                                 cstring::to_cstring(first_idx + tag - first_tag) +
                                                 "]." + field_name;
                        auto *field = phvInfo.field(new_field_name);
                        slice = new PHV::FieldSlice(field, extract_slice->range());
                    }

                    Clot::FieldKind kind;
                    if (clotInfo.is_checksum(slice))
                        kind = Clot::FieldKind::CHECKSUM;
                    else if (clotInfo.is_modified(slice))
                        kind = Clot::FieldKind::MODIFIED;
                    else if (clotInfo.is_readonly(slice))
                        kind = Clot::FieldKind::READONLY;
                    else
                        kind = Clot::FieldKind::UNUSED;

                    for (const auto &state : extract->states()) {
                        cstring state_name = clotInfo.sanitize_state_name(state->name, gress);
                        clot->add_slice(state_name, kind, slice);
                        if (LOGGING(4)) {
                            std::string kind_str;
                            switch (kind) {
                                case Clot::FieldKind::CHECKSUM:
                                    kind_str = "checksum field";
                                    break;
                                case Clot::FieldKind::MODIFIED:
                                    kind_str = "modified phv field";
                                    break;
                                case Clot::FieldKind::READONLY:
                                    kind_str = "read-only phv field";
                                    break;
                                case Clot::FieldKind::UNUSED:
                                    kind_str = "field";
                                    break;
                            }
                            LOG4("  Added " << kind_str << " " << slice->shortString() << " at bit "
                                            << clot->bit_offset(state_name, slice)
                                            << " in parser state " << state_name);
                        }
                    }
                }

                if (depth > 1) {
                    clot->stack_depth = depth - static_cast<int>(first_idx);
                    clot->stack_inc = delta;
                }
                free_tags.erase(tag);
                tag += delta;
            }

            // Done allocating the candidate. Remove any candidates that would violate
            // CLOT-allocation limits, and adjust the rest to account for the inter-CLOT gap
            // requirement.
            auto new_candidates = new ClotCandidateSet();
            auto num_clots_allocated = clotInfo.num_clots_allocated(gress);
            BUG_CHECK(num_clots_allocated <= MAX_CLOTS_PER_GRESS,
                      "%d CLOTs allocated in %s, when at most %d are allowed", num_clots_allocated,
                      toString(gress), MAX_CLOTS_PER_GRESS);
            if (clotInfo.num_clots_allocated(gress) == MAX_CLOTS_PER_GRESS) {
                // We've allocated the last CLOT available in the gress. Remove all other
                // candidates for the gress.
                for (auto candidate : *candidates) {
                    if (candidate->thread() != gress) new_candidates->insert(candidate);
                }
            } else {
                auto graph = parserInfo.graphs().at(parserInfo.parser(*states.begin()));
                auto full_states = clotInfo.find_full_states(graph);
                if (!full_states->empty()) {
                    if (LOGGING(6)) {
                        LOG6("The following states are now full.");
                        LOG6(
                            "Allocating more CLOTs would violate the max-CLOTs-per-packet "
                            "constraint.");

                        for (auto state : *full_states) LOG6("  " << state->name);

                        LOG6("");
                    }
                }

                // Set of removed candidates; used for logging.
                ClotCandidateSet removed_candidates;

                for (auto other_candidate : *candidates) {
                    if (intersects(*full_states, other_candidate->states())) {
                        // Candidate has a full state. Remove from candidacy.
                        if (LOGGING(3)) removed_candidates.insert(other_candidate);
                        continue;
                    }

                    if (candidate == other_candidate) continue;

                    auto adjusted =
                        adjust_for_allocation(other_candidate, candidate, header_removals);
                    new_candidates->insert(adjusted->begin(), adjusted->end());
                }

                if (LOGGING(3) && !removed_candidates.empty()) {
                    LOG3(
                        "Removed the following from candidacy to satisfy the "
                        "max-CLOTs-per-packet constraint:");
                    for (auto candidate : removed_candidates) {
                        LOG3("  Candidate " << candidate->id);

                        bool first_state = true;
                        for (auto *state : candidate->states()) {
                            LOG3("  " << (first_state ? "states: " : "        ") << state->name);
                            first_state = false;
                        }
                    }
                    LOG3("");
                }
            }

            candidates = new_candidates;

            if (LOGGING(3)) {
                LOG3("");
                if (candidates->empty()) {
                    LOG3("CLOT allocation complete: no remaining CLOT candidates.");
                } else {
                    LOG3(candidates->size() << " remaining CLOT candidates:");
                    for (auto candidate : *candidates) LOG3(logfix(candidate->print()));
                }
            }
        }
    }

    HeaderValidityAnalysis::ResultMap analyze_header_removals(const ClotCandidateSet *candidates,
                                                              const IR::MAU::TableSeq *mau) {
        // Build the set of correlations that we're interested in.
        std::set<FieldSliceSet> correlations = {};
        for (const auto *c1 : *candidates) {
            for (const auto *c2 : *candidates) {
                if (c1 == c2) continue;

                // Only interesting if c1 and c2 can appear back-to-back in the packet.
                if (!c1->byte_gaps(parserInfo, c2).count(0) &&
                    !c2->byte_gaps(parserInfo, c1).count(0))
                    continue;

                for (const auto *pov1 : c1->pov_bits) {
                    for (const auto *pov2 : c2->pov_bits) {
                        FieldSliceSet set = {pov1, pov2};
                        correlations.emplace(set);
                    }
                }
            }
        }

        HeaderValidityAnalysis hva(phvInfo, correlations);
        mau->apply(hva);
        return hva.resultMap;
    }

    void filter_stack_extracts(FieldExtractInfo *field_extract_info) {
        // Identify pseudoheaders with header stacks
        std::map<cstring, std::map<unsigned, const Pseudoheader *>> pseudoheader_stacks;
        for (const auto &[pseudoheader, extracts] : field_extract_info->pseudoheaderMap) {
            for (const auto *field : Keys(extracts)) {
                if (auto stack_index = get_element_index(field)) {
                    cstring header_name = field->header();
                    header_name = header_name.before(strrchr(header_name, '['));
                    pseudoheader_stacks[header_name].emplace(*stack_index, pseudoheader);
                    LOG5("Pseudoheader " << pseudoheader->id << " extracts " << header_name << "["
                                         << *stack_index << "]");
                }
            }
        }

        if (LOGGING(6)) {
            for (auto &[stack, index_map] : pseudoheader_stacks) {
                Log::TempIndent indent;
                LOG6("Indeces/pseudoheaders for Header stack " << stack << ":" << indent);
                for (auto &[index, pseudoheader] : index_map) {
                    LOG6("    " << index << " in " << pseudoheader->id);
                }
            }
        }

        // Identify pseudoheaders to be eliminated because a subset of
        // indices are missing for a state
        bool done = false;
        auto header_stack_map = field_extract_info->headerStackMap;
        ordered_set<const Pseudoheader *> pseudoheaders_to_remove;
        while (!done) {
            done = true;
            for (auto &[stack, index_map] : pseudoheader_stacks) {
                ordered_set<unsigned> indices_to_remove;
                BUG_CHECK(header_stack_map.count(stack),
                          "Header stack %1% missing from CLOT header stack map");
                for (auto &[index, pseudoheader] : index_map) {
                    bool failed = false;
                    for (auto &[_, state_indices] : header_stack_map.at(stack)) {
                        if (state_indices.count(index)) {
                            for (auto state_index : state_indices) {
                                if (!index_map.count(state_index)) {
                                    pseudoheaders_to_remove.emplace(pseudoheader);
                                    failed = true;
                                    break;
                                }
                            }
                        }
                        if (failed) {
                            indices_to_remove.emplace(index);
                            break;
                        }
                    }
                }
                if (indices_to_remove.size()) {
                    done = false;
                    for (auto index : indices_to_remove) index_map.erase(index);
                }
            }
        }

        // Remove the identified pseudoheaders
        for (auto pseudoheader : pseudoheaders_to_remove) {
            field_extract_info->pseudoheaderMap.erase(pseudoheader);
            LOG4("Removing pseudoheader " << pseudoheader->id);
        }
    }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        // Configure logging for this visitor.
        if (BackendOptions().verbose > 0) {
            if (auto pipe = root->to<IR::BFN::Pipe>())
                log = new Logging::FileLog(pipe->canon_id(), "clot_allocation.log"_cs);
        }

        // Make sure we clear our state from previous invocations of the visitor.
        auto result = Visitor::init_apply(root);
        clear();
        return result;
    }

    const IR::Node *apply_visitor(const IR::Node *root, const char *) override {
        const auto *pipe = root->to<IR::BFN::Pipe>();
        BUG_CHECK(pipe, "GreedyClotAllocator must be applied to pipe");

        // Loop over each gress.
        for (const auto &entry : parserInfo.graphs()) {
            const auto *parser = entry.first;
            const auto *graph = entry.second;

            // Build auxiliary data structures.
            auto field_extract_info = group_extracts(graph);
            if (LOGGING(4)) {
                LOG4("Extracts found that can be part of a CLOT:");
                for (const auto &entry2 : field_extract_info->pseudoheaderMap) {
                    const auto *pseudoheader = entry2.first;
                    const auto &extracts = entry2.second;

                    LOG4("  In pseudoheader " << pseudoheader->id << ":");
                    for (const auto *field : Keys(extracts)) LOG4("    " << field->name);
                    LOG4("");
                }
                LOG4("");
            }
            filter_stack_extracts(field_extract_info);

            // Identify CLOT candidates (using older pseudoheader technique)
            auto candidates = find_clot_candidates(field_extract_info->pseudoheaderMap);
            // Identify additional CLOT candidates (using deparser analysis)
            const auto *deparser = pipe->thread[parser->gress].deparser->to<IR::BFN::Deparser>();
            const auto *mau = pipe->thread[parser->gress].mau;
            HeaderValidityAnalysis hva(phvInfo, {});
            mau->apply(hva);
            auto sequences = find_multiheader_sequences(deparser, field_extract_info->fieldMap,
                                                        hva.povBitsSetInvalidInMau,
                                                        hva.povBitsAlwaysInvalidateTogether);
            for (auto &sequence : sequences)
                if (!sequence.empty()) try_add_clot_candidate(candidates, nullptr, sequence);

            if (LOGGING(3)) {
                if (candidates->empty()) {
                    LOG3("No CLOT candidates found.");
                } else {
                    LOG3(candidates->size() << " CLOT candidates:");
                    for (auto candidate : *candidates) LOG3(logfix(candidate->print()));
                }
            }

            // Do header-removal analysis.
            auto header_removals = analyze_header_removals(candidates, mau);

            // Perform allocation.
            allocate(candidates, header_removals);
        }

        if (logAllocation)
            if (auto *pipe = root->to<IR::BFN::Pipe>())
                Logging::FileLog parserLog(pipe->canon_id(), "clot_allocation.log"_cs);

        LOG2(clotInfo.print());

        return root;
    }

    void end_apply() override {
        if (log) Logging::FileLog::close(log);
        Visitor::end_apply();
    }

    void clear() {}

    const std::set<const PHV::Field *> get_fields_emitted_more_than_once(
        const IR::BFN::Deparser *deparser) {
        std::set<const PHV::Field *> fields_emitted;
        std::set<const PHV::Field *> fields_emitted_more_than_once;
        for (auto emit : deparser->emits) {
            if (auto emit_field = emit->to<IR::BFN::EmitField>()) {
                const PHV::Field *field = phvInfo.field(emit_field->source->field);
                auto pair = fields_emitted.insert(field);
                if (pair.second == true) continue;
                fields_emitted_more_than_once.insert(field);
            }
        }
        return fields_emitted_more_than_once;
    }

    std::vector<std::vector<const FieldSliceExtractInfo *>> find_multiheader_sequences(
        const IR::BFN::Deparser *deparser,
        ordered_map<const PHV::Field *, FieldSliceExtractInfo *> field_map,
        std::set<const PHV::Field *> pov_bits_set_invalid_in_mau,
        SymBitMatrix pov_bits_always_set_invalid_together) {
        LOG4("Finding multiheader extract sequences based on deparser emits");

        const auto *clot_eligible_fields = clotInfo.clot_eligible_fields();
        auto is_clot_eligible = [&clot_eligible_fields](const PHV::Field *field) -> bool {
            return clot_eligible_fields->find(field) != clot_eligible_fields->end();
        };

        const auto fields_emitted_more_than_once = get_fields_emitted_more_than_once(deparser);
        auto is_emitted_more_than_once =
            [&fields_emitted_more_than_once](const PHV::Field *field) -> bool {
            return fields_emitted_more_than_once.count(field);
        };

        std::vector<const IR::BFN::EmitChecksum *> checksum_updates;
        auto get_checksum_updates = [&](const PHV::Field *field) {
            return clotInfo.field_to_checksum_updates().count(field)
                       ? clotInfo.field_to_checksum_updates().at(field)
                       : std::vector<const IR::BFN::EmitChecksum *>();
        };

        std::vector<std::vector<const FieldSliceExtractInfo *>> sequences;
        std::vector<const FieldSliceExtractInfo *> sequence;
        std::map<const IR::BFN::ParserState *, int> state_to_size_in_bits;
        const PHV::Field *prev_pov_bit = nullptr;

        for (auto emit : deparser->emits) {
            const PHV::Field *field = nullptr;
            const PHV::Field *pov_bit = nullptr;
            if (auto emit_field = emit->to<IR::BFN::EmitField>()) {
                field = phvInfo.field(emit_field->source->field);
                if (auto alias_slice = emit_field->povBit->field->to<IR::BFN::AliasSlice>())
                    pov_bit = phvInfo.field(alias_slice->source);
                else
                    pov_bit = phvInfo.field(emit_field->povBit->field);
            }

            // Do not consider fields that are ineligible for multiheader CLOTs.
            auto set_invalid_in_mau_compatible = [&](const PHV::Field *pov_bit,
                                                     const PHV::Field *prev_pov_bit) {
                return sequence.empty() || prev_pov_bit == pov_bit ||
                       (pov_bit && prev_pov_bit &&
                        (pov_bits_always_set_invalid_together(pov_bit->id, prev_pov_bit->id) ||
                         (!pov_bits_set_invalid_in_mau.count(pov_bit) &&
                          !pov_bits_set_invalid_in_mau.count(prev_pov_bit))));
            };
            if (field == nullptr || !is_clot_eligible(field) || is_emitted_more_than_once(field) ||
                clotInfo.is_extracted_in_multiple_non_mutex_states(pov_bit) ||
                !set_invalid_in_mau_compatible(pov_bit, prev_pov_bit)) {
                std::string prefix = sequence.empty() ? "" : "Ended sequence: ";
                if (field == nullptr && LOGGING(4)) {
                    LOG4("  " << prefix << "Emit is not an EmitField");
                } else if (LOGGING(4)) {
                    std::string bullet = "\n    - ";
                    std::stringstream reasons;
                    if (!is_clot_eligible(field))
                        reasons << bullet << field->name << " is not CLOT eligible";
                    if (is_emitted_more_than_once(field))
                        reasons << bullet << field->name << " is emitted more than once";
                    if (clotInfo.is_extracted_in_multiple_non_mutex_states(pov_bit))
                        reasons << bullet << pov_bit->name << " is extracted in multiple "
                                << "mutually exclusive parser states";
                    if (pov_bits_set_invalid_in_mau.count(pov_bit))
                        reasons << bullet << pov_bit->name << " is set in MAU";

                    LOG4("  " << prefix << field->name << " is not eligible for "
                              << "multiheader extract sequences" << reasons.str());
                }

                if (!sequence.empty()) {
                    sequences.push_back(sequence);
                    sequence.clear();
                    prev_pov_bit = nullptr;
                    state_to_size_in_bits.clear();
                    checksum_updates.clear();
                }
                if (field == nullptr || !is_clot_eligible(field) ||
                    is_emitted_more_than_once(field) ||
                    clotInfo.is_extracted_in_multiple_non_mutex_states(pov_bit))
                    continue;
            }

            // Additionally, each eligible field in a multiheader CLOT must be used
            // in the same checksum updates.
            if (!sequence.empty() && !checksum_updates.empty() &&
                !get_checksum_updates(field).empty() &&
                checksum_updates != get_checksum_updates(field)) {
                LOG4("  Ended sequence: " << field->name
                                          << " is involved in different checksum "
                                             "updates than previous field in sequence");
                sequences.push_back(sequence);
                sequence.clear();
                prev_pov_bit = nullptr;
                state_to_size_in_bits.clear();
                checksum_updates = get_checksum_updates(field);
            } else if (checksum_updates.empty()) {
                checksum_updates = get_checksum_updates(field);
            }

            auto try_start_new_sequence = [&](FieldSliceExtractInfo *extract_info) {
                bool can_start_new_sequence = clotInfo.can_start_clot(extract_info) &&
                                              parserInfo.graph(*extract_info->states().begin())
                                                  .is_mutex(extract_info->states());
                state_to_size_in_bits.clear();
                if (can_start_new_sequence) {
                    LOG4("  Started a sequence with field: " << field->name);
                    sequence.push_back(extract_info);
                    prev_pov_bit = pov_bit;
                    for (const auto &state : extract_info->states())
                        state_to_size_in_bits[state] = extract_info->slice()->size();
                } else {
                    LOG4("  Cannot start a new sequence: "
                         << field->name << " cannot start a CLOT or is extracted in parser states "
                         << "that are not mutually exclusive.");
                }
            };

            auto extract_info = field_map[field];
            if (sequence.empty()) {
                try_start_new_sequence(extract_info);
            } else if (!sequence.empty()) {
                // Special case for back to back emits of the same checksum PHV::Field*
                if (*extract_info == *sequence.back()) continue;

                if (contains(sequence.front()->states(), extract_info->states())) {
                    bool extracts_are_adjacent_in_all_states = true;
                    for (const auto &state : extract_info->states()) {
                        auto previous_extract_info =
                            std::find_if(sequence.rbegin(), sequence.rend(),
                                         [&state](const FieldSliceExtractInfo *extract_info) {
                                             return extract_info->states().count(state);
                                         });
                        unsigned expected_bit_offset =
                            (*previous_extract_info)->state_bit_offset(state) +
                            (*previous_extract_info)->slice()->size();
                        if (extract_info->state_bit_offset(state) != expected_bit_offset) {
                            extracts_are_adjacent_in_all_states = false;
                            break;
                        }
                    }
                    if (extracts_are_adjacent_in_all_states) {
                        LOG4("    Added " << field->name << " to sequence");
                        sequence.push_back(extract_info);
                        if (pov_bit) prev_pov_bit = pov_bit;
                        for (const auto &state : extract_info->states())
                            state_to_size_in_bits[state] += extract_info->slice()->size();
                    } else {
                        LOG4("  Ended sequence: extracts for "
                             << field->name
                             << " are not adjacent to the last field in the sequence in all parser "
                                "states");
                        sequences.push_back(sequence);
                        sequence.clear();
                        prev_pov_bit = nullptr;
                        try_start_new_sequence(extract_info);
                    }
                } else {
                    LOG4("  Ended sequence: "
                         << field->name
                         << " is not extracted in the same parser states as the first field in "
                            "the sequence");
                    sequences.push_back(sequence);
                    sequence.clear();
                    prev_pov_bit = nullptr;
                    try_start_new_sequence(extract_info);
                }
            }

            const int MAX_SIZE = Device::pardeSpec().byteMaxClotSize() * 8;
            auto it =
                std::find_if(state_to_size_in_bits.begin(), state_to_size_in_bits.end(),
                             [&MAX_SIZE](const std::pair<const IR::BFN::ParserState *, int> &pair) {
                                 return pair.second > MAX_SIZE;
                             });

            if (it != state_to_size_in_bits.end()) {
                sequence.pop_back();
                sequences.push_back(sequence);
                sequence.clear();
                prev_pov_bit = nullptr;
                LOG4("  Ended sequence: sequence is larger than " << MAX_SIZE / 8 << " bytes");
                if (extract_info->slice()->size() < MAX_SIZE) {
                    try_start_new_sequence(extract_info);
                }
            }
        }
        if (!sequence.empty()) {
            sequences.push_back(sequence);
            sequence.clear();
            prev_pov_bit = nullptr;
        }

        LOG4("Multiheader extract sequences found: " << sequences.size());
        for (const auto &sequence : sequences) {
            std::stringstream out;
            out << "[ ";
            std::string delimiter = "";
            for (const auto &extract : sequence) {
                out << delimiter << extract->slice()->shortString();
                delimiter = ", ";
            }
            out << " ]";
            LOG4("  " << out);
            LOG4("");
        }
        return sequences;
    }
};

AllocateClot::AllocateClot(ClotInfo &clotInfo, const PhvInfo &phv, PhvUse &uses,
                           PragmaDoNotUseClot &pragmaDoNotUseClot, PragmaAlias &pragmaAlias,
                           bool log)
    : clotInfo(clotInfo) {
    addPasses({
        &uses,
        &clotInfo.parserInfo,
        LOGGING(3) ? new DumpParser("before_clot_allocation") : nullptr,
        /// FieldPovAnalysis runs a data flow analysis on parser and fills result in
        /// clotInfo.pov_extracted_without_fields
        new FieldPovAnalysis(clotInfo, phv),
        new CollectClotInfo(phv, clotInfo, pragmaDoNotUseClot),
        new GreedyClotAllocator(phv, clotInfo, log),
        new CheckClotGroups(phv, clotInfo),
        LOGGING(3) ? new DumpParser("after_clot_allocation") : nullptr,
        &pragmaAlias,  // Rerun because the field IDs are out-of-date
        new MergeDesugaredVarbitValids(phv, clotInfo, pragmaAlias),
    });
}

Visitor::profile_t ClotAdjuster::init_apply(const IR::Node *root) {
    auto result = Visitor::init_apply(root);

    // Configure logging for this visitor.
    if (BackendOptions().verbose > 0) {
        if (auto pipe = root->to<IR::BFN::Pipe>())
            log = new Logging::FileLog(pipe->canon_id(), "clot_allocation.log"_cs);
    }

    return result;
}

const IR::Node *ClotAdjuster::apply_visitor(const IR::Node *root, const char *) {
    clotInfo.adjust_clots(phv);

    const IR::BFN::Pipe *pipe = root->to<IR::BFN::Pipe>();
    if (pipe) Logging::FileLog parserLog(pipe->canon_id(), "clot_allocation.log"_cs);
    LOG1(clotInfo.print(&phv));

    return root;
}

void ClotAdjuster::end_apply(const IR::Node *) {
    if (log) Logging::FileLog::close(log);
    Visitor::end_apply();
}
