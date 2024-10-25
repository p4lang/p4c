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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_EXTRACT_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_EXTRACT_INFO_H_

#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"
#include "clot_candidate.h"
#include "lib/ordered_map.h"

using StatePair = std::pair<const IR::BFN::ParserState *, const IR::BFN::ParserState *>;
using StatePairSet = ordered_set<StatePair>;

/// Holds information relating to a field slice's extract.
class FieldSliceExtractInfo {
    friend class GreedyClotAllocator;

    /// The parser states in which the field slice is extracted, mapped to the field slice's offset
    /// (in bits) from the start of the state.
    // Invariant: all offsets here are congruent, modulo 8.
    ordered_map<const IR::BFN::ParserState *, unsigned> state_bit_offsets_;

    /// By stack offset: the parser states in which the field slice is extracted, mapped
    /// to the field slice's offset (in bits) from the start of the state.
    // Invariant: all offsets here are congruent, modulo 8.
    std::map<unsigned, ordered_map<const IR::BFN::ParserState *, unsigned>>
        stack_offset_state_bit_offsets_;

    /// The field slice's minimum offset, in bits, from the start of the packet.
    int min_packet_bit_offset_;

    /// The field slice's maximum offset, in bits, from the start of the packet.
    int max_packet_bit_offset_;

    /// The field slice itself.
    const PHV::FieldSlice *slice_;

 public:
    FieldSliceExtractInfo(
        const ordered_map<const IR::BFN::ParserState *, unsigned> &state_bit_offsets,
        const std::map<unsigned, ordered_map<const IR::BFN::ParserState *, unsigned>>
            &stack_offset_state_bit_offsets,
        int min_packet_bit_offset, int max_packet_bit_offset, const PHV::Field *field)
        : FieldSliceExtractInfo(state_bit_offsets, stack_offset_state_bit_offsets,
                                min_packet_bit_offset, max_packet_bit_offset,
                                new PHV::FieldSlice(field)) {}

    FieldSliceExtractInfo(
        const ordered_map<const IR::BFN::ParserState *, unsigned> &state_bit_offsets,
        const std::map<unsigned, ordered_map<const IR::BFN::ParserState *, unsigned>>
            &stack_offset_state_bit_offsets,
        int min_packet_bit_offset, int max_packet_bit_offset, const PHV::FieldSlice *slice)
        : state_bit_offsets_(state_bit_offsets),
          stack_offset_state_bit_offsets_(stack_offset_state_bit_offsets),
          min_packet_bit_offset_(min_packet_bit_offset),
          max_packet_bit_offset_(max_packet_bit_offset),
          slice_(slice) {}

    /// Updates this object for this field being extracted in a newly discovered parser state.
    ///
    /// @param state
    ///             the new parser state
    /// @param stack_offset
    ///             stack index offset
    /// @param state_bit_offset
    ///             the field's bit offset from the beginning of @p state
    /// @param min_packet_bit_offset
    ///             the minimum bit offset from the beginning of the packet of the field in the
    ///             given state
    /// @param max_packet_bit_offset
    ///             the maximum bit offset from the beginning of the packet of the field in the
    ///             given state
    void update(const IR::BFN::ParserState *state, unsigned stack_offset, unsigned state_bit_offset,
                int min_packet_bit_offset, int max_packet_bit_offset);

    /// @return the parser states in which the field slice is extracted.
    ordered_set<const IR::BFN::ParserState *> states() const;

    /// @return the field slice's minimum offset, in bits, from the start of the packet to the
    /// start of the field slice.
    int min_packet_bit_offset() const { return min_packet_bit_offset_; }

    /// @return the field slice's maximum offset, in bits, from the start of the packet to the
    /// start of the field slice.
    int max_packet_bit_offset() const { return max_packet_bit_offset_; }

    /// @return the field slice's offset, in bits, from the start of each parser state in which the
    /// field is extracted.
    const ordered_map<const IR::BFN::ParserState *, unsigned> &state_bit_offsets() const {
        return state_bit_offsets_;
    }

    /// @return the bit offset of the field slice, relative to the given parser @p state.
    unsigned state_bit_offset(const IR::BFN::ParserState *state) const {
        return state_bit_offsets_.at(state);
    }

    /// @return the field slice's bit-in-byte offset in the packet.
    unsigned bit_in_byte_offset() const {
        auto offset = *Values(state_bit_offsets_).begin();
        return offset % 8;
    }

    /// @return the field slice itself.
    const PHV::FieldSlice *slice() const { return slice_; }

    /// Trims the start of the extract so that it is byte-aligned.
    const FieldSliceExtractInfo *trim_head_to_byte() const;

    /// Trims the start of the extract so that it does not extend past the minimum CLOT position.
    const FieldSliceExtractInfo *trim_head_to_min_clot_pos() const;

    /// Trims the end of the extract so that it is byte-aligned.
    const FieldSliceExtractInfo *trim_tail_to_byte() const;

    /// Trims the end of the extract so that it does not extend past the maximum CLOT position.
    const FieldSliceExtractInfo *trim_tail_to_max_clot_pos() const;

 private:
    /// Trims the given number of bits off the end of the extract.
    const FieldSliceExtractInfo *trim_tail_bits(int size) const;

 public:
    /// Trims the extract to a sub-slice.
    ///
    /// @param lo_idx The start of the new slice, relative to the start of the old slice.
    /// @param bits The number of bits.
    ///
    /// Slices are trimmed relative to the least-significant bit (LSB) of the slice. A @p lo_idx
    /// value of 0 leaves the slice LSB unchanged, while a positive value discards @p lo_idx LSBs
    /// from the slice.
    const FieldSliceExtractInfo *trim(int lo_idx, int bits) const;

    /// Removes any bytes that conflict with the given @p candidate, returning the resulting list
    /// of FieldSliceExtractInfo instances, in the order in which they appear in the packet. More
    /// than one instance can result if conflicting bytes occur in the middle of the extract.
    ///
    /// @param parserInfo  Parser info.
    /// @param preGapBits  The size, in bits, of the inter-CLOT gap required before the given
    ///                    @p candidate.
    /// @param candidate   CLOT candidate.
    /// @param postGapBits The size, in bits, of the inter-CLOT gap required after the given
    ///                   @p candidate.
    std::vector<const FieldSliceExtractInfo *> *remove_conflicts(
        const CollectParserInfo &parserInfo, int preGapBits, const ClotCandidate *candidate,
        int postGapBits) const;

    /// Analogous to ClotCandidate::gaps. Produces a map wherein the keys are all possible gap
    /// sizes, in bits, between the end of this extract and the start of another extract when this
    /// extract occurs before the other extract in the input packet. An error occurs if any
    /// possible gap is not a multiple of 8 bits.
    ///
    /// Each possible gap size is mapped to the set of parser states that realize that gap. Each
    /// set member is a pair, wherein the first component is the state containing this extract, and
    /// the second component is the state containing @p other.
    const std::map<unsigned, StatePairSet> byte_gaps(const CollectParserInfo &parserInfo,
                                                     const FieldSliceExtractInfo *other) const;

    /// Produces a map wherein the keys are all possible gap sizes, in bits, between the end of
    /// this extract and the start of another extract when this extract occurs before the other
    /// extract in the input packet.
    ///
    /// Each possible gap size is mapped to the set of parser states that realize that gap. Each
    /// set member is a pair, wherein the first component is the state containing this extract, and
    /// the second component is the state containing @p other.
    const std::map<unsigned, StatePairSet> bit_gaps(const CollectParserInfo &parserInfo,
                                                    const FieldSliceExtractInfo *other) const;

    friend bool operator==(const FieldSliceExtractInfo &, const FieldSliceExtractInfo &);

 private:
    friend std::ostream &operator<<(std::ostream &, const FieldSliceExtractInfo &);
    friend std::ostream &operator<<(std::ostream &, const FieldSliceExtractInfo *);
};

/// Summarizes parser extracts for all fields.
class FieldExtractInfo {
 public:
    typedef std::map<const Pseudoheader *, ordered_map<const PHV::Field *, FieldSliceExtractInfo *>,
                     Pseudoheader::Less>
        PseudoheaderMap;
    typedef ordered_map<const PHV::Field *, FieldSliceExtractInfo *> FieldMap;
    typedef std::map<cstring, std::map<const IR::BFN::ParserState *, std::set<unsigned>>>
        HeaderStackMap;

    /// Maps pseudoheaders to fields to their FieldSliceExtractInfo instances.
    PseudoheaderMap pseudoheaderMap;

    /// Maps all extracted fields to their FieldSliceExtractInfo instances.
    FieldMap fieldMap;

    /// Maps all header stacks to the sets of indicies that could be extracted in each state
    // FIXME: Don't need to make sure that all or none of the indices extracted in a state
    // are covered by CLOTs. If multiple are extracted in a single iteration, then potentionally
    // only those at the same offset across all iterationsd need to be CLOTed/not CLOTed.
    // e.g., if a state extracts 0 & 1, or 2 & 3, then it's probably okay if only 0 and 2 are
    // CLOTed because they are both the first offset in the state being processed.
    HeaderStackMap headerStackMap;

    void updateFieldMap(const PHV::Field *field, const IR::BFN::ParserState *state,
                        unsigned stack_offset, unsigned state_bit_offset, int min_packet_bit_offset,
                        int max_packet_bit_offset) {
        if (fieldMap.count(field)) {
            fieldMap.at(field)->update(state, stack_offset, state_bit_offset, min_packet_bit_offset,
                                       max_packet_bit_offset);
        } else {
            ordered_map<const IR::BFN::ParserState *, unsigned> state_bit_offsets;
            state_bit_offsets[state] = state_bit_offset;
            std::map<unsigned, ordered_map<const IR::BFN::ParserState *, unsigned>>
                stack_offset_state_bit_offsets;
            stack_offset_state_bit_offsets[stack_offset][state] = state_bit_offset;
            fieldMap[field] =
                new FieldSliceExtractInfo(state_bit_offsets, stack_offset_state_bit_offsets,
                                          min_packet_bit_offset, max_packet_bit_offset, field);
        }
    }

    void updatePseudoheaderMap(const Pseudoheader *pseudoheader, const PHV::Field *field,
                               const IR::BFN::ParserState *state, unsigned stack_offset,
                               unsigned state_bit_offset, int min_packet_bit_offset,
                               int max_packet_bit_offset) {
        if (pseudoheaderMap.count(pseudoheader) && pseudoheaderMap.at(pseudoheader).count(field)) {
            auto *fei = pseudoheaderMap.at(pseudoheader).at(field);
            fei->update(state, stack_offset, state_bit_offset, min_packet_bit_offset,
                        max_packet_bit_offset);
        } else {
            ordered_map<const IR::BFN::ParserState *, unsigned> state_bit_offsets;
            state_bit_offsets[state] = state_bit_offset;
            std::map<unsigned, ordered_map<const IR::BFN::ParserState *, unsigned>>
                stack_offset_state_bit_offsets;
            stack_offset_state_bit_offsets[stack_offset][state] = state_bit_offset;
            auto fei =
                new FieldSliceExtractInfo(state_bit_offsets, stack_offset_state_bit_offsets,
                                          min_packet_bit_offset, max_packet_bit_offset, field);

            pseudoheaderMap[pseudoheader][field] = fei;
        }
    }

    void updateHeaderStackMap(cstring header_stack, const IR::BFN::ParserState *state,
                              const std::set<unsigned> indices) {
        headerStackMap[header_stack][state].insert(indices.begin(), indices.end());
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_EXTRACT_INFO_H_ */
