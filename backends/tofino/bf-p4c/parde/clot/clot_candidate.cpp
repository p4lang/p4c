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

#include "clot_candidate.h"

#include "bf-p4c/common/table_printer.h"
#include "field_slice_extract_info.h"

unsigned ClotCandidate::max_size_in_bits() const {
    auto it =
        std::max_element(parser_state_to_size_in_bits.begin(), parser_state_to_size_in_bits.end(),
                         [](const std::pair<const IR::BFN::ParserState *, unsigned> &a,
                            const std::pair<const IR::BFN::ParserState *, unsigned> &b) {
                             return a.second < b.second;
                         });
    BUG_CHECK(it != parser_state_to_size_in_bits.end(),
              "Cannot get CLOT candidate %1%'s max size, as it is not in any parser states", id);
    return it->second;
}

unsigned ClotCandidate::size_in_bits(const IR::BFN::ParserState *parser_state) const {
    BUG_CHECK(parser_state_to_size_in_bits.count(parser_state),
              "Cannot get CLOT candidate %1%'s size, as it is not in parser state %2%", id,
              parser_state->name);
    return parser_state_to_size_in_bits.at(parser_state);
}

ClotCandidate::ClotCandidate(const ClotInfo &clotInfo, const Pseudoheader *pseudoheader,
                             const std::vector<const FieldSliceExtractInfo *> &extract_infos,
                             bool afterAllocatedClot, bool beforeAllocatedClot)
    : pseudoheader(pseudoheader),
      extract_infos(extract_infos),
      id(nextId++),
      afterAllocatedClot(afterAllocatedClot),
      beforeAllocatedClot(beforeAllocatedClot) {
    unsigned offset = 0;
    unsigned idx = 0;

    if (pseudoheader != nullptr) {
        pov_bits = pseudoheader->pov_bits;
    } else {
        auto first_field = extract_infos.front()->slice()->field();
        pov_bits = clotInfo.fields_to_pov_bits_.at(first_field);
    }

    for (const auto *parser_state : extract_infos.front()->states()) {
        parser_state_to_checksum_bits[parser_state];
        parser_state_to_modified_bits[parser_state];
        parser_state_to_readonly_bits[parser_state];
        parser_state_to_unused_bits[parser_state];
        parser_state_to_size_in_bits[parser_state] = 0;
    }

    for (const auto *extract_info : extract_infos) {
        const PHV::FieldSlice *slice = extract_info->slice();
        for (const auto *parser_state : extract_info->states()) {
            if (clotInfo.is_checksum(slice)) {
                parser_state_to_checksum_bits[parser_state].setrange(offset, slice->size());
                checksum_slices.insert(slice);
            }

            if (clotInfo.is_modified(slice)) {
                parser_state_to_modified_bits[parser_state].setrange(offset, slice->size());
                modified_slices.insert(slice);
            }

            if (clotInfo.is_readonly(slice)) {
                parser_state_to_readonly_bits[parser_state].setrange(offset, slice->size());
                readonly_slices.insert(slice);
            }

            if (clotInfo.is_unused(slice)) {
                parser_state_to_unused_bits[parser_state].setrange(offset, slice->size());
                unused_slices.insert(slice);
            }
            parser_state_to_size_in_bits[parser_state] += slice->size();
        }

        if (clotInfo.can_start_clot(extract_info)) can_start_indices_.push_back(idx);

        if (clotInfo.can_end_clot(extract_info)) can_end_indices_.push_back(idx);

        offset += slice->size();
        idx++;
    }

    auto get_sum_of_bits = [](const std::set<const PHV::FieldSlice *> &slices) -> unsigned {
        return std::accumulate(
            slices.begin(), slices.end(), 0u,
            [](unsigned sum, const PHV::FieldSlice *slice) { return sum + slice->size(); });
    };
    checksum_bits = get_sum_of_bits(checksum_slices);
    modified_bits = get_sum_of_bits(modified_slices);
    readonly_bits = get_sum_of_bits(readonly_slices);
    unused_bits = get_sum_of_bits(unused_slices);
    std::reverse(can_end_indices_.begin(), can_end_indices_.end());
}

ordered_set<const IR::BFN::ParserState *> ClotCandidate::states() const {
    return (*(extract_infos.begin()))->states();
}

gress_t ClotCandidate::thread() const { return (*Keys(state_bit_offsets()).begin())->thread(); }

const ordered_map<const IR::BFN::ParserState *, unsigned> &ClotCandidate::state_bit_offsets()
    const {
    return (*(extract_infos.begin()))->state_bit_offsets();
}

unsigned ClotCandidate::bit_in_byte_offset() const {
    return (*(extract_infos.begin()))->bit_in_byte_offset();
}

const std::vector<const FieldSliceExtractInfo *> &ClotCandidate::extracts() const {
    return extract_infos;
}

const std::vector<unsigned> &ClotCandidate::can_start_indices() const { return can_start_indices_; }

const std::vector<unsigned> &ClotCandidate::can_end_indices() const { return can_end_indices_; }

const std::map<unsigned, StatePairSet> ClotCandidate::byte_gaps(const CollectParserInfo &parserInfo,
                                                                const ClotCandidate *other) const {
    std::map<unsigned, StatePairSet> byte_gaps;
    for (const auto *state : states()) {
        auto it = std::find_if(extract_infos.rbegin(), extract_infos.rend(),
                               [&state](const FieldSliceExtractInfo *extract_info) {
                                   return extract_info->states().count(state);
                               });
        std::map<unsigned, StatePairSet> byte_gaps_for_state =
            (*it)->byte_gaps(parserInfo, other->extract_infos.front());

        if (byte_gaps_for_state.empty()) continue;

        std::map<unsigned, StatePairSet> matches;
        std::copy_if(byte_gaps_for_state.begin(), byte_gaps_for_state.end(),
                     std::inserter(matches, matches.begin()),
                     [&state](const std::pair<unsigned, StatePairSet> &kv) {
                         auto it = std::find_if(kv.second.begin(), kv.second.end(),
                                                [&state](const StatePair &state_pair) {
                                                    return state_pair.first == state ||
                                                           state_pair.second == state;
                                                });
                         return it != kv.second.end();
                     });

        for (const auto &match : matches) {
            for (const auto &state_pair : match.second) {
                byte_gaps[match.first].insert(state_pair);
            }
        }
    }
    return byte_gaps;
}

const ClotCandidate *ClotCandidate::mark_adjacencies(bool afterAllocatedClot,
                                                     bool beforeAllocatedClot) const {
    afterAllocatedClot |= this->afterAllocatedClot;
    beforeAllocatedClot |= this->beforeAllocatedClot;

    if (afterAllocatedClot == this->afterAllocatedClot &&
        beforeAllocatedClot == this->beforeAllocatedClot)
        return this;

    auto *result = new ClotCandidate(*this);
    result->afterAllocatedClot = afterAllocatedClot;
    result->beforeAllocatedClot = beforeAllocatedClot;
    return result;
}

bool ClotCandidate::operator<(const ClotCandidate &other) const {
    if (unused_bits != other.unused_bits) return unused_bits < other.unused_bits;

    if (readonly_bits != other.readonly_bits) return readonly_bits < other.readonly_bits;

    return id < other.id;
}

std::string ClotCandidate::print() const {
    std::stringstream out;
    out << "CLOT candidate " << id << ":" << std::endl;

    if (afterAllocatedClot) out << "  Appears after an allocated CLOT with 0-byte gap" << std::endl;
    if (beforeAllocatedClot)
        out << "  Appears before an allocated CLOT with 0-byte gap" << std::endl;

    TablePrinter tp(out, {"Parser State", "Fields", "Bits", "Property"},
                    TablePrinter::Align::CENTER);

    ordered_set<const IR::BFN::ParserState *> parser_states = extract_infos.front()->states();
    bool first_parser_state = true;
    for (const auto *parser_state : parser_states) {
        if (!first_parser_state) tp.addSep();
        first_parser_state = false;

        int offset = 0;
        bool first_extract_info = true;
        for (const auto *extract_info : extract_infos) {
            const PHV::FieldSlice *slice = extract_info->slice();
            ordered_set<const IR::BFN::ParserState *> subset_of_parser_states =
                extract_info->states();

            if (!subset_of_parser_states.count(parser_state)) continue;

            std::stringstream bits;
            bits << slice->size() << " [" << offset << ".." << (offset + slice->size() - 1) << "]";

            std::string attr;
            if (parser_state_to_checksum_bits.at(parser_state).getbit(offset)) attr = "checksum";
            if (parser_state_to_modified_bits.at(parser_state).getbit(offset))
                attr = "modified";
            else if (parser_state_to_readonly_bits.at(parser_state).getbit(offset))
                attr = "read-only";
            else
                attr = "unused";

            tp.addRow({std::string(first_extract_info ? parser_state->name : ""),
                       std::string(slice->shortString()), bits.str(), attr});
            first_extract_info = false;

            offset += slice->size();
        }
        tp.addBlank();
        tp.addRow({"", "Unused bits",
                   std::to_string(parser_state_to_unused_bits.at(parser_state).popcount()), ""});
        tp.addRow({"", "Read-only bits",
                   std::to_string(parser_state_to_readonly_bits.at(parser_state).popcount()), ""});
        tp.addRow({"", "Modified bits",
                   std::to_string(parser_state_to_modified_bits.at(parser_state).popcount()), ""});
        tp.addRow({"", "Checksum bits",
                   std::to_string(parser_state_to_checksum_bits.at(parser_state).popcount()), ""});
        tp.addRow({"", "Total bits", std::to_string(offset), ""});
    }

    tp.addSep();
    tp.addRow({"", "Unique unused bits", std::to_string(unused_bits), ""});
    tp.addRow({"", "Unique read-only bits", std::to_string(readonly_bits), ""});
    tp.addRow({"", "Unique modified bits", std::to_string(modified_bits), ""});
    tp.addRow({"", "Unique checksum bits", std::to_string(checksum_bits), ""});
    tp.addRow({"", "Max total bits", std::to_string(max_size_in_bits()), ""});

    tp.print();

    return out.str();
}

unsigned ClotCandidate::nextId = 0;
