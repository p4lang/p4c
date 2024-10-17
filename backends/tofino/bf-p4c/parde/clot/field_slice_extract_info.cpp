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

#include "field_slice_extract_info.h"

void FieldSliceExtractInfo::update(const IR::BFN::ParserState* state,
                                   unsigned stack_offset, unsigned state_bit_offset,
                                   int min_packet_offset, int max_packet_offset) {
    BUG_CHECK(!stack_offset_state_bit_offsets_[stack_offset].count(state),
              "Field %s is unexpectedly extracted multiple times in %s", slice_->field()->name,
              state->name);

    if (stack_offset_state_bit_offsets_[stack_offset].size() == 0) {
        stack_offset_state_bit_offsets_[stack_offset].emplace(state, state_bit_offset);
    } else {
        auto& entry = *state_bit_offsets_.begin();
        BUG_CHECK(entry.second % 8 == state_bit_offset % 8,
                  "Field %s determined to be CLOT-eligible, but has inconsistent bit-in-byte "
                  "offsets in states %s and %s",
                  slice_->field()->name, entry.first->name, state->name);

        BUG_CHECK(entry.first->thread() == state->thread(),
                  "A FieldSliceExtractInfo for an %s extract of field %s is being updated with an "
                  "extract in parser state %s, which comes from %s",
                  toString(entry.first->thread()), slice_->field()->name, state->name,
                  toString(state->thread()));
    }

    state_bit_offsets_[state] = state_bit_offset;
    min_packet_bit_offset_ = std::min(min_packet_bit_offset_, min_packet_offset);
    max_packet_bit_offset_ = std::max(max_packet_bit_offset_, max_packet_offset);
}

ordered_set<const IR::BFN::ParserState*> FieldSliceExtractInfo::states() const {
    ordered_set<const IR::BFN::ParserState*> result;
    for (auto state : Keys(state_bit_offsets_)) result.insert(state);
    return result;
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim_head_to_byte() const {
    auto trim_amount = (8 - bit_in_byte_offset()) % 8;
    auto size = slice_->size() - trim_amount;
    BUG_CHECK(size > 0, "Trimmed extract %1% to %2% bits", slice()->shortString(), size);
    return trim(0, size);
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim_head_to_min_clot_pos() const {
    gress_t gress = slice()->field()->gress;
    int trim_amount =
        static_cast<int>(Device::pardeSpec().bitMinClotPos(gress)) - min_packet_bit_offset_;
    if (trim_amount <= 0) return this;
    BUG_CHECK(slice()->size() > trim_amount,
              "Cannot trim field slice by amount greater than the size of the field slice.");

    return trim(0, slice()->size() - trim_amount);
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim_tail_to_byte() const {
    auto trim_amount = (bit_in_byte_offset() + slice_->size()) % 8;
    return trim_tail_bits(trim_amount);
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim_tail_to_max_clot_pos() const {
    int max_pos = max_packet_bit_offset_ + slice_->size();
    int trim_amount = max_pos - Device::pardeSpec().bitMaxClotPos();
    if (trim_amount <= 0) return this;

    return trim_tail_bits(trim_amount);
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim_tail_bits(int trim_amt) const {
    auto start_idx = trim_amt;
    auto size = slice_->size() - trim_amt;
    BUG_CHECK(size > 0, "Trimmed extract %1% to %2% bits", slice()->shortString(), size);
    return trim(start_idx, size);
}

const FieldSliceExtractInfo* FieldSliceExtractInfo::trim(int lo_idx, int bits) const {
    BUG_CHECK(lo_idx >= 0,
              "Attempted to trim an extract of %s to a negative start index: %d",
              slice_->shortString(), lo_idx);

    auto cur_size = slice_->size();
    BUG_CHECK(cur_size >= lo_idx + bits - 1,
              "Attempted to trim an extract of %s to a sub-slice larger than the original "
              "(lo_idx = %d, bits = %d)",
              slice_->shortString(), lo_idx, bits);

    if (lo_idx == 0 && cur_size == bits) return this;

    int adjustment = cur_size - lo_idx - bits;

    int min_packet_bit_offset = min_packet_bit_offset_ + adjustment;
    int max_packet_bit_offset = max_packet_bit_offset_ + adjustment;

    ordered_map<const IR::BFN::ParserState*, unsigned> state_bit_offsets;
    for (auto& [state, bit_offset] : state_bit_offsets_)
        state_bit_offsets[state] = bit_offset + adjustment;

    std::map<unsigned, ordered_map<const IR::BFN::ParserState*, unsigned>>
        stack_offset_state_bit_offsets;
    for (auto& [stack_offset, state_bit_offsets] : stack_offset_state_bit_offsets_)
        for (auto& [state, bit_offset] : state_bit_offsets)
            stack_offset_state_bit_offsets[stack_offset][state] = bit_offset + adjustment;

    auto range = slice_->range().shiftedByBits(lo_idx).resizedToBits(bits);
    auto slice = new PHV::FieldSlice(slice_->field(), range);
    return new FieldSliceExtractInfo(state_bit_offsets, stack_offset_state_bit_offsets,
                                     min_packet_bit_offset, max_packet_bit_offset, slice);
}

std::vector<const FieldSliceExtractInfo*>*
FieldSliceExtractInfo::remove_conflicts(const CollectParserInfo& parserInfo,
                                        int preGapBits,
                                        const ClotCandidate* candidate,
                                        int postGapBits) const {
    int extract_size = slice()->size();

    // For each parser state in which this field slice is extracted, and each parser state
    // corresponding to the candidate, each path through the parser yields a possibly different
    // shift amount to get from the parser state of the extract to that of the candidate, so the
    // candidate can appear in a few different positions relative to the extract. Go through the
    // various parser states and shift amounts and figure out which bits conflict.
    //
    // Using coordinates relative to the start of the extract, for each shift amount, the candidate
    // starts at bit (shift + candidate_offset - extract_offset). So, each shift amount results in
    // a conflict with the bits in the interval
    //
    //   [shift + candidate_offset - extract_offset - preGapBits,
    //    shift + candidate_offset - extract_offset + candidate_size + postGapBits - 1].
    //
    // So, we have a conflict for shift amounts in the interval
    //
    //   [extract_offset - candidate_offset - candidate_size - postGapBits + 1,
    //    extract_offset + extract_size - candidate_offset + preGapBits - 1].

    // Tracks which bits in the extract conflict with the candidate. Indices are in field order,
    // which is the opposite of packet order.
    bitvec conflicting_bits;

    for (auto& extract_entry : state_bit_offsets()) {
        auto& extract_state = extract_entry.first;
        int extract_offset = extract_entry.second;

        for (auto& candidate_entry : candidate->state_bit_offsets()) {
            auto& candidate_state = candidate_entry.first;
            int candidate_offset = candidate_entry.second;

            auto shift_amounts = parserInfo.get_all_shift_amounts(extract_state, candidate_state);

            int candidate_size = candidate->size_in_bits(candidate_state);
            int lower_bound = extract_offset - candidate_offset - candidate_size - postGapBits + 1;
            int upper_bound = extract_offset + extract_size - candidate_offset + preGapBits - 1;

            for (auto it = shift_amounts->lower_bound(lower_bound);
                 it != shift_amounts->end() && *it <= upper_bound;
                 ++it) {
                auto shift = *it;

                auto conflict_start =
                    std::max(0, shift + candidate_offset - extract_offset - preGapBits);
                auto conflict_end =
                    std::min(extract_size - 1,
                             shift + candidate_offset - extract_offset
                                 + candidate_size + postGapBits - 1);
                auto conflict_size = conflict_end - conflict_start + 1;

                // NB: conflict_start and conflict_end are in packet coordinates, but
                // conflicting_bits uses field coordinates, so we do the conversion here.
                conflicting_bits.setrange(extract_size - conflict_end - 1, conflict_size);
            }
        }
    }

    if (!conflicting_bits.popcount()) return new std::vector<const FieldSliceExtractInfo*>({this});

    auto result = new std::vector<const FieldSliceExtractInfo*>();

    // Scan through conflicting_bits and build our results list.
    int start_idx = -1;
    for (int idx = extract_size - 1; idx >= 0; --idx) {
        if (start_idx != -1 && conflicting_bits[idx]) {
            // Finished a chunk of non-conflicts.
            result->push_back(trim(idx + 1, start_idx - idx));
            start_idx = -1;
        }

        if (start_idx == -1 && !conflicting_bits[idx]) {
            // Starting a new chunk of non-conflicts.
            start_idx = idx;
        }
    }

    if (start_idx != -1) {
        // Add a final chunk of non-conflicts.
        result->push_back(trim(0, start_idx + 1));
    }

    return result;
}

const std::map<unsigned, StatePairSet>
FieldSliceExtractInfo::byte_gaps(const CollectParserInfo& parserInfo,
                                 const FieldSliceExtractInfo* other) const {
    std::map<unsigned, StatePairSet> result;

    for (auto& entry : bit_gaps(parserInfo, other)) {
        auto& bit_gap = entry.first;
        auto& states = entry.second;

        BUG_CHECK(bit_gap % 8 == 0, "Gap between %1% in state %2% and %3% in state %4% "
            "can be %5% bits, which is not a whole-byte multiple",
            slice_->shortString(),
            states.begin()->first,
            other->slice_->shortString(),
            states.begin()->second,
            bit_gap);

        result[static_cast<unsigned>(bit_gap / 8)] = states;
    }

    return result;
}

const std::map<unsigned, StatePairSet>
FieldSliceExtractInfo::bit_gaps(const CollectParserInfo& parserInfo,
                                const FieldSliceExtractInfo* other) const {
    std::map<unsigned, StatePairSet> result;

    for (const auto& thisEntry : state_bit_offsets_) {
        const auto* thisState = thisEntry.first;
        const auto thisStateBitOffset = thisEntry.second;

        for (const auto& otherEntry : other->state_bit_offsets_) {
            const auto* otherState = otherEntry.first;
            const auto otherStateBitOffset = otherEntry.second;

            for (auto shiftAmount : *parserInfo.get_all_shift_amounts(thisState, otherState)) {
                int gap = shiftAmount + otherStateBitOffset - thisStateBitOffset - slice_->size();
                if (gap >= 0) {
                    result[static_cast<unsigned>(gap)].insert({thisState, otherState});
                }
            }
        }
    }

    return result;
}

bool operator==(const FieldSliceExtractInfo& a, const FieldSliceExtractInfo& b) {
    return a.state_bit_offsets_ == b.state_bit_offsets_ &&
           a.min_packet_bit_offset_ == b.min_packet_bit_offset_ &&
           a.max_packet_bit_offset_ == b.max_packet_bit_offset_ && *a.slice_ == *b.slice_;
}

std::ostream& operator<<(std::ostream& out, const FieldSliceExtractInfo& field_slice_extract_info) {
    out << "{ Field Slice: " << field_slice_extract_info.slice_->shortString() << ", ";

    out << "Min Packet Bit Offset: " << field_slice_extract_info.min_packet_bit_offset_ << ", ";
    out << "Max Packet Bit Offset: " << field_slice_extract_info.max_packet_bit_offset_ << ", ";

    std::string delimiter = "";
    out << "State Bit Offsets: [ ";
    for (const auto& [key, value] : field_slice_extract_info.state_bit_offsets_) {
        out << delimiter << "( " << key->name << ", " << value << " )";
        delimiter = ", ";
    }
    out << " ] }";

    return out;
}

std::ostream& operator<<(std::ostream& out,
                         const FieldSliceExtractInfo* field_slice_extract_info) {
    if (field_slice_extract_info) return out << *field_slice_extract_info;
    return out << "(nullptr)";
}
