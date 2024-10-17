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

#include "clot_info.h"

#include <array>
#include <string>
#include <vector>
#include <initializer_list>

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/device.h"
#include "bf-p4c/logging/logging.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/bitvec.h"
#include "clot_candidate.h"
#include "field_slice_extract_info.h"

std::optional<nw_bitrange> ClotInfo::field_range(const IR::BFN::ParserState* state,
                                                   const PHV::Field* field) const {
    BUG_CHECK(field_to_parser_states_.count(field),
              "Field %1% is never extracted from the packet",
              field->name);
    BUG_CHECK(field_to_parser_states_.at(field).count(state),
              "Parser state %1% does not extract %2%",
              state->name, field->name);

    if (field_range_.count(state) && field_range_.at(state).count(field))
        return field_range_.at(state).at(field);

    return std::optional<nw_bitrange>{};
}

std::optional<unsigned> ClotInfo::offset(const IR::BFN::ParserState* state,
                                           const PHV::Field* field) const {
    if (auto range = field_range(state, field))
        return range->lo;

    return std::optional<unsigned>{};
}

cstring ClotInfo::sanitize_state_name(cstring state_name, gress_t gress) const {
    // To have a stable lookup, we require state_name to be always prepended
    // by a gress

    std::stringstream ss;
    ss << gress;

    // It is likely the state name is that of a split state,
    // e.g. Original state = ingress::stateA
    //      Split state = ingress::stateA.$common.0
    // The parser_state_to_clots_ map is created before splitting and
    // carries the original state name. We regenerate the original state
    // name by stripping out the split name addition ".$common.0"

    std::string st(state_name.c_str());
    auto pos = st.find(".$");

    // Even though we want to strip off split information, we want to keep
    // loop unroll iteration counts
    auto it_pos = st.find(".$it");
    std::string it_str;
    if (it_pos != std::string::npos) {
        auto it_str_untrimmed = st.substr(it_pos + 1, st.length());
        auto pos = it_str_untrimmed.find(".");
        it_str = "." + it_str_untrimmed.substr(0, pos);
    }

    return (!state_name.startsWith(ss.str())  // if not prefixed with gress
        ? (ss.str() + "::") : "") + st.substr(0, pos)  // add the prefix, cut generated appendix
        + it_str;  // add any iteration number
}

const Clot* ClotInfo::parser_state_to_clot(const IR::BFN::LoweredParserState *state,
                                           unsigned tag) const {
    auto state_name = sanitize_state_name(state->name, state->thread());
    auto& parser_state_to_clots = this->parser_state_to_clots(state->thread());

    if (parser_state_to_clots.count(state_name)) {
        auto& clots = parser_state_to_clots.at(state_name);
        auto it = std::find_if(clots.begin(), clots.end(), [&](const Clot* sclot) {
            return (sclot->tag == tag); });
        if (it != clots.end()) return *it;
    }

    return nullptr;
}

std::map<int, PHV::Container>
ClotInfo::get_overwrite_containers(const Clot* clot, const PhvInfo& phv) const {
    std::map<int, PHV::Container> containers;

    for (const auto& kv : clot->parser_state_to_slices()) {
        cstring parser_state = kv.first;
        const std::vector<const PHV::FieldSlice*> slices = kv.second;
        for (const auto* slice : slices) {
            auto field = slice->field();
            auto range = slice->range();
            if (slice_overwritten_by_phv(phv, clot, slice)) {
                field->foreach_alloc(range, PHV::AllocContext::DEPARSER, nullptr,
                        [&](const PHV::AllocSlice &alloc) {
                    auto container = alloc.container();
                    auto net_range = range.toOrder<Endian::Network>(field->size);
                    int field_in_clot_offset = clot->bit_offset(parser_state, slice) - net_range.lo;

                    auto field_range = alloc.field_slice().toOrder<Endian::Network>(field->size);

                    auto container_range =
                        alloc.container_slice().toOrder<Endian::Network>(alloc.container().size());

                    auto container_offset = field_in_clot_offset - container_range.lo +
                                            field_range.lo;

                    BUG_CHECK(container_offset % 8 == 0,
                            "CLOT %d container overwrite offset for %s is not byte-aligned",
                            clot->tag,
                            slice->shortString());

                    auto container_offset_in_byte = container_offset / 8;

                    if (containers.count(container_offset_in_byte)) {
                        auto other_container = containers.at(container_offset_in_byte);
                        BUG_CHECK(container == other_container,
                            "CLOT %d has more than one container at overwrite offset %d: "
                            "%s and %s",
                            clot->tag,
                            container_offset_in_byte,
                            container,
                            other_container);
                    } else {
                        containers[container_offset_in_byte] = container;
                    }
                });
            }
        }
    }

    return containers;
}

std::map<int, const PHV::Field*> ClotInfo::get_csum_fields(const Clot* clot) const {
    std::map<int, const PHV::Field*> checksum_fields;
    for (const auto& kv : clot->parser_state_to_slices()) {
        cstring parser_state = kv.first;
        for (const auto* checksum_field : clot->checksum_fields()) {
            auto offset = clot->byte_offset(parser_state, new PHV::FieldSlice(checksum_field));
            if (checksum_fields.count(offset)) {
                auto other_field = checksum_fields.at(offset);
                BUG_CHECK(checksum_field == other_field,
                    "CLOT %d has more than one checksum field at overwrite offset %d: %s and %s",
                    checksum_field->name, other_field->name);
            }

            checksum_fields[offset] = checksum_field;
        }
    }

    return checksum_fields;
}

void ClotInfo::merge_parser_states(gress_t gress, cstring dst_state_name, cstring src_state_name) {
    auto src_name = sanitize_state_name(src_state_name, gress);
    auto dst_name = sanitize_state_name(dst_state_name, gress);

    auto& state_to_clot = parser_state_to_clots(gress);
    if (!state_to_clot.count(src_name)) return;  // No clot extracts here

    // Bind clots from src to dst
    // dst might not even exist, but this will create appropriate record
    auto& src_clots = state_to_clot.at(src_name);
    for (const auto* clot : src_clots) {
        parser_state_to_clots(gress)[dst_name].insert(clot);
        clot_to_parser_states_[clot].first = gress;
        clot_to_parser_states_[clot].second.insert(dst_name);
    }
}

void ClotInfo::add_field(const PHV::Field* f, const IR::BFN::ParserRVal* source,
                         const IR::BFN::ParserState* state) {
    LOG4("adding " << f->name << " to " << state->name << " (source " << source << ")");
    parser_state_to_fields_[state].push_back(f);
    field_to_parser_states_[f][state] = source;

    if (auto rval = source->to<IR::BFN::PacketRVal>())
        field_range_[state][f] = rval->range;
}

void ClotInfo::add_stack(const IR::HeaderStack* hs,
                         const IR::BFN::ParserState* state,
                         int index) {
    std::string extra = index >= 0 ? " (element " + std::to_string(index) + ")" : "";
    LOG4("adding stack " << hs << " to " << state->name << extra);
    parser_state_to_header_stacks_[state].push_back(hs);
    if (index >= 0)
        header_stack_elements_[std::make_pair(state, hs)].emplace(static_cast<unsigned>(index));
}

void ClotInfo::compute_byte_maps() {
    for (auto& [state, fields_in_state] : parser_state_to_fields_) {
        for (auto f : fields_in_state) {
            if (auto f_offset_opt = offset(state, f)) {
                unsigned f_offset = *f_offset_opt;

                unsigned start_byte =  f_offset / 8;
                unsigned end_byte = (f_offset + f->size - 1) / 8;

                for (unsigned i = start_byte; i <= end_byte; i++) {
                    field_to_byte_idx[state][f].insert(i);
                    byte_idx_to_field[state][i].insert(f);
                }
            }
        }
    }

    if (LOGGING(4)) {
        std::clog << "=====================================================" << std::endl;

        for (auto& [state, field_byte_idx_map] : field_to_byte_idx) {
            std::clog << "state: " << state->name << std::endl;
            for (auto& [field, indices] : field_byte_idx_map) {
                std::clog << "  " << field->name << " in byte";
                for (auto id : indices)
                    std::clog << " " << id;
                std::clog << std::endl;
            }
        }

        std::clog << "-----------------------------------------------------" << std::endl;

        for (auto& [state, byte_idx_field_map] : byte_idx_to_field) {
            std::clog << "state: " << state->name << std::endl;
            for (auto& [byte, fields] : byte_idx_field_map) {
                std::clog << "  Byte " << byte << " has:";
                for (auto* f : fields)
                    std::clog << " " << f->name;
                std::clog << std::endl;
            }
        }

        std::clog << "=====================================================" << std::endl;
    }
}

void ClotInfo::add_clot(Clot* clot, ordered_set<const IR::BFN::ParserState*> states) {
    clots_.push_back(clot);

    std::set<cstring> state_names;
    for (auto* state : states) {
        auto state_name = sanitize_state_name(state->name, state->thread());
        state_names.insert(state_name);
        parser_state_to_clots_[clot->gress][state_name].insert(clot);
    }

    clot_to_parser_states_[clot] = {clot->gress, state_names};
}

// DANGER: This method assumes the parser graph is a DAG.
const ordered_set<const IR::BFN::ParserState*>* ClotInfo::find_full_states(
        const IR::BFN::ParserGraph* graph) const {
    auto root = graph->root;
    auto gress = root->thread();
    const auto MAX_LIVE_CLOTS = Device::pardeSpec().maxClotsLivePerGress();
    ordered_set<const IR::BFN::ParserState*>* result;

    // Find states that are part of paths on which the maximum number of live CLOTs are allocated.
    if (num_clots_allocated(gress) < MAX_LIVE_CLOTS) {
        result = new ordered_set<const IR::BFN::ParserState*>();
    } else {
        auto largest = find_largest_paths(parser_state_to_clots(gress), graph, root);
        BUG_CHECK(largest->first <= MAX_LIVE_CLOTS,
            "Packet has %d live CLOTs, when at most %d are allowed",
            largest->first,
            MAX_LIVE_CLOTS);

        result =
            largest->first < MAX_LIVE_CLOTS
                ? new ordered_set<const IR::BFN::ParserState*>()
                : largest->second;
    }

    // Enforcement of the CLOTs-per-state constraint is done in a later pass, during state
    // splitting.

    return result;
}

bool ClotInfo::is_used_in_multiple_checksum_update_sets(const PHV::Field* field) const {
     if (!field_to_checksum_updates_.count(field))
         return false;

      return field_to_checksum_updates_.at(field).size() > 1;

     // TODO it's probably still ok to allocate field to CLOT
     // if the checksum updates it is involved in are such that
     // one's source list is a subset of the update?
}

bool ClotInfo::is_extracted_in_multiple_non_mutex_states(const PHV::Field* f) const {
    // If we have no extracts for the field (or no states that extract the field), then it's
    // vacuously true that all extracts are in mutually exclusive states.
    if (!field_to_parser_states_.count(f)) return false;

    auto& states_to_sources = field_to_parser_states_.at(f);
    if (states_to_sources.size() == 0) return false;

    // Collect the states in a set and check that they all come from the same parser.
    const IR::BFN::Parser* parser = nullptr;
    ordered_set<const IR::BFN::ParserState*> states;
    for (auto state : Keys(states_to_sources)) {
        states.insert(state);

        auto* cur_parser = parserInfo.parser(state);
        if (parser == nullptr) {
            parser = cur_parser;
            BUG_CHECK(parser, "An extract of %1% was associated with a null parser", f);
        }

        BUG_CHECK(parser == cur_parser,
                  "Extracts of %1% don't all come from the same parser",
                  f);
    }

    // See if these states are mutually exclusive.
    return !parserInfo.graph(parser).is_mutex(states);
}

bool ClotInfo::has_consistent_bit_in_byte_offset(const PHV::Field* field) const {
    if (!field_to_parser_states_.count(field))
        // Field not extracted, so vacuously true.
        return true;

    unsigned bit_in_byte_offset = 0;
    bool have_offset = false;
    for (auto& state : Keys(field_to_parser_states_.at(field))) {
        auto cur_bit_in_byte_offset = *offset(state, field) % 8;
        if (!have_offset) {
            bit_in_byte_offset = cur_bit_in_byte_offset;
            have_offset = true;
        }

        if (cur_bit_in_byte_offset != bit_in_byte_offset) return false;
    }

    return true;
}
bool ClotInfo::extracted_with_pov(const PHV::Field* field) const {
    if (!fields_to_pov_bits_.count(field))
        return true;
    for (auto pov : fields_to_pov_bits_.at(field)) {
        if (pov_extracted_without_fields.count(pov->field())) {
            return false;
        }
    }
    return true;
}

bool ClotInfo::can_be_in_clot(const PHV::Field* field) const {
    if (do_not_use_clot_fields.count(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: "
             << "it is marked by @pragma do_not_use_clot");
        return false;
    }

    if (is_added_by_mau(field->header())) {
        LOG5("  Field " << field->name << " can't be in a CLOT: its header might be added by MAU");
        return false;
    }

    if (!field->emitted() && !is_checksum(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: not emitted and not a checksum");
        return false;
    }

    if (is_used_in_multiple_checksum_update_sets(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: it's involved in multiple "
             "checksum-update field lists");
        return false;
    }

    if (is_extracted_in_multiple_non_mutex_states(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: it's extracted in multiple parser "
             "states");
        return false;
    }

    if (!extracts_full_width(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: its full width is not extracted "
             "from the packet by the parser");
        return false;
    }

    if (!has_consistent_bit_in_byte_offset(field)) {
        LOG5("  Field " << field->name << " can't be in a CLOT: it has different bit-in-byte "
             "offsets in different parser states");
        return false;
    }

    if (!extracted_with_pov(field)) {
        LOG5(" Field " << field->name << " can't be in a CLOT: there is exists a path through "
             "the parser where POV bit of this field is set, but the field is not extracted");
        return false;
    }

    return true;
}

bool ClotInfo::is_slice_below_min_offset(const PHV::FieldSlice* slice,
                                         int min_packet_bit_offset) const {
    int bit_min_clot_position =
        static_cast<int>(Device::pardeSpec().bitMinClotPos(slice->field()->gress));
    if (min_packet_bit_offset < bit_min_clot_position) {
        // If the size of the slice is greater than difference between bit_min_clot_position and
        // min_packet_bit_offset, then the slice can be trimed later
        if (slice->size() <= (bit_min_clot_position - min_packet_bit_offset)) return true;
    }
    return false;
}

bool ClotInfo::can_start_clot(const FieldSliceExtractInfo* extract_info) const {
    auto slice = extract_info->slice();

    // Either the start of the slice is byte-aligned, or the slice contains a byte boundary.
    int offset_from_byte = extract_info->bit_in_byte_offset();
    if (offset_from_byte != 0 && slice->size() < 9 - offset_from_byte) {
        LOG6("  Can't start CLOT with " << slice->shortString() << ": not byte-aligned, and "
             "doesn't contain byte boundary");
        return false;
    }

    // Slice must not be modified.
    if (is_modified(slice)) {
        LOG6("  Can't start CLOT with " << slice->field()->name << ": is modified");
        return false;
    }

    // Slice must not be a checksum.
    if (is_checksum(slice)) {
        LOG6("  Can't start CLOT with " << slice->field()->name << ": is checksum");
        return false;
    }

    if (is_slice_below_min_offset(slice, extract_info->min_packet_bit_offset())) {
        LOG6("  Can't start CLOT with " << slice->field()->name << " : min offset"
             " is less than hdr_len_adj");
        return false;
    }
    return true;
}

bool ClotInfo::can_end_clot(const FieldSliceExtractInfo* extract_info) const {
    auto slice = extract_info->slice();

    // Either the end of the slice is byte-aligned, or the slice contains a byte boundary.
    int offset_from_byte = (extract_info->bit_in_byte_offset() + slice->size()) % 8;
    if (offset_from_byte != 0 && slice->size() < offset_from_byte + 1) {
        LOG6("  Can't end CLOT with " << slice->shortString() << ": not byte-aligned, and "
             "doesn't contain byte boundary");
        return false;
    }

    // Slice must not be modified.
    if (is_modified(slice)) {
        LOG6("  Can't end CLOT with " << slice->field()->name << ": is modified");
        return false;
    }

    // Slice must not be a checksum.
    if (is_checksum(slice)) {
        LOG6("  Can't end CLOT with " << slice->field()->name << ": is checksum");
        return false;
    }

    // Slice must start before the maximum CLOT position.
    if (extract_info->max_packet_bit_offset() >=
        static_cast<int>(Device::pardeSpec().bitMaxClotPos())) {
        LOG6("  Can't end CLOT with " << slice->field()->name << ": start offset "
                                      << extract_info->max_packet_bit_offset() << " not less than "
                                      << Device::pardeSpec().bitMaxClotPos());
        return false;
    }

    return true;
}

bool ClotInfo::extracts_full_width(const PHV::Field* field) const {
    if (!field_to_parser_states_.count(field))
        return false;

    bool have_packet_extract = false;
    for (auto& entry : field_to_parser_states_.at(field)) {
        auto& state = entry.first;
        auto& source = entry.second;

        if (!source->to<IR::BFN::PacketRVal>()) return false;

        if (auto range = field_range(state, field)) {
            have_packet_extract = true;
            if (field->size != range->size()) return false;
        }
    }

    return have_packet_extract;
}

bool ClotInfo::is_checksum(const PHV::Field* field) const {
    return checksum_dests_.count(field);
}

bool ClotInfo::is_checksum(const PHV::FieldSlice* slice) const {
    return is_checksum(slice->field());
}

bool ClotInfo::is_modified(const PHV::Field* field) const {
    if (is_modified_.count(field)) return is_modified_.at(field);

    if (uses.is_written_mau(field)) return is_modified_[field] = true;

    // Tie off infinite recursion by first assuming that the field is not modified.
    is_modified_[field] = false;

    // Recursively check if the field is packed with a modified field in the same header.
    if (field_to_parser_states_.count(field)) {
        for (auto& entry : field_to_parser_states_.at(field)) {
            auto& state = entry.first;

            if (!field_to_byte_idx.count(state)) continue;
            auto byte_indices = field_to_byte_idx.at(state);

            if (!byte_indices.count(field)) continue;
            for (auto idx : byte_indices.at(field)) {
                for (auto other_field : byte_idx_to_field.at(state).at(idx)) {
                    if (field->header() == other_field->header() && is_modified(other_field)) {
                        return is_modified_[field] = true;
                    }
                }
            }
        }
    }

    // Recursively check if the field is aliased with a modified field.
    if (field_aliases_.count(field)) {
        // TODO: Treat all aliases as modified for now. For some reason, uses.is_written_mau()
        // is returning false for aliases that are written by MAU.
#ifdef __XXX_ALIASES_NOT_WORKING
        for (auto alias : *(field_aliases_.at(field))) {
            if (is_modified(alias)) {
                return is_modified_[field] = true;
            }
        }
#else
        return is_modified_[field] = true;
#endif
    }

    return false;
}

bool ClotInfo::is_modified(const PHV::FieldSlice* slice) const {
    return is_modified(slice->field());
}

bool ClotInfo::is_readonly(const PHV::Field* field) const {
    return uses.is_used_mau(field) && !is_modified(field) && !is_checksum(field);
}

bool ClotInfo::is_readonly(const PHV::FieldSlice* slice) const {
    return is_readonly(slice->field());
}

bool ClotInfo::is_unused(const PHV::Field* field) const {
    return !uses.is_used_mau(field) && !is_modified(field) && !is_checksum(field);
}

bool ClotInfo::is_unused(const PHV::FieldSlice* slice) const {
    return is_unused(slice->field());
}

const std::set<const PHV::Field*>* ClotInfo::clot_eligible_fields() const {
    auto result = new std::set<const PHV::Field*>();
    for (auto entry : field_to_parser_states_) {
        auto field = entry.first;
        if (can_be_in_clot(field)) result->insert(field);
    }
    return result;
}

std::set<const Clot*, Clot::Less>* ClotInfo::fully_allocated(const PHV::FieldSlice* slice) const {
    auto allocated = allocated_slices(slice);
    if (!allocated) return nullptr;

    // Go through the CLOT allocation for the given slice and collect up the set of CLOTs. As
    // we do this, figure out which bits in the slice are CLOT-allocated.
    auto result = new std::set<const Clot*, Clot::Less>();
    bitvec clotted_bits;

    for (const auto& entry : *allocated) {
        const auto& cur_slice = entry.first;
        const auto& cur_clot = entry.second;

        auto clotted_range = cur_slice->range().intersectWith(slice->range());
        clotted_bits.setrange(clotted_range.lo, clotted_range.size());

        result->insert(cur_clot);
    }

    bitvec expected(slice->range().lo, slice->range().size());
    if (clotted_bits != expected) {
        // Whole slice not CLOT-allocated.
        return nullptr;
    }

    return result;
}

void ClotInfo::adjust_clots(const PhvInfo& phv) {
    std::vector<Clot*> clots;
    for (auto clot : clots_) {
        if (adjust(phv, clot)) {
            // Adjustment successful, so keep the CLOT.
            clots.push_back(clot);
        } else {
            // Adjustment resulted in an empty CLOT, so remove it.
            auto& pair = clot_to_parser_states_.at(clot);
            auto gress = pair.first;
            auto state_names = pair.second;
            clot_to_parser_states_.erase(clot);

            auto& parser_state_to_clots = this->parser_state_to_clots(gress);
            for (auto state_name : state_names) {
                auto& state_clots = parser_state_to_clots.at(state_name);
                state_clots.erase(clot);
                if (state_clots.empty()) parser_state_to_clots.erase(state_name);
            }
        }
    }

    clots_ = clots;
}

bool ClotInfo::adjust(const PhvInfo& phv, Clot* clot) {
    Log::TempIndent indent;
    LOG3("Adjusting clot " << *clot << indent);
    unsigned length_in_bytes = 0;

    // Step 1: Identify states which are "non-terminal" CLOT states.
    // Non-terminal CLOT states are defined as follows:
    //  - We have two states: A and B.
    //  - State A's CLOT ends with header field f1.
    //  - State B's CLOT contains header field f1, but it is not the final field.
    //  - In this case, A is a non-terminal state.
    std::set<cstring> non_terminal_states;
    std::set<const PHV::FieldSlice*> non_final_slices;

    // Step 1a: identify the non-final field slices
    for (const auto& [parser_state, slices] : clot->parser_state_to_slices()) {
        LOG5("  Parser state to slices: " << parser_state << " : " << slices);
        if (slices.empty()) continue;
        for (auto it = slices.begin(); it != std::prev(slices.end()); ++it) {
            LOG5("    Parser state to non final slices: " << *it);
            non_final_slices.emplace(*it);
        }
    }
    LOG4("Non Final Slices:" << non_final_slices);

    // Step 1b: identify the states that are non-terminal
    for (const auto& [parser_state, slices] : clot->parser_state_to_slices()) {
        if (!slices.empty() && non_final_slices.count(slices.back())) {
            LOG4("  State " << parser_state << " is non-terminal for CLOT tag " << clot->tag);
            non_terminal_states.emplace(parser_state);
        }
    }
    LOG4("Non Terminal States:" << non_terminal_states);

    // Step 2: Do the trimming
    for (const auto& [parser_state, slices] : clot->parser_state_to_slices()) {
        LOG5("  Parser State: " << parser_state << ", slices: " << slices);
        if (non_terminal_states.count(parser_state)) continue;

        // Figure out how many bits are overwritten at the start of the CLOT.
        unsigned num_start_bits_overwritten = 0;
        for (auto slice : slices) {
            LOG5("    Computing start bits overwritten For slice: " << slice);
            auto overwrite_mask = bits_overwritten(phv, clot, slice);
            int first_zero_idx = overwrite_mask.ffz(0);
            num_start_bits_overwritten += first_zero_idx;
            LOG5("      overwrite_mask: " << overwrite_mask
                    << ", first_zero_idx: " << first_zero_idx
                    << ", num_start_bits_overwritten: " << num_start_bits_overwritten);
            if (first_zero_idx < slice->size()) break;
        }
        LOG5("  num_start_bits_overwritten:" << num_start_bits_overwritten);

        BUG_CHECK(num_start_bits_overwritten % 8 == 0,
                  "CLOT %d starts with %d overwritten bits, which is not byte-aligned", clot->tag,
                  num_start_bits_overwritten);

        // Figure out how many bits are overwritten at the end of the CLOT.
        unsigned num_end_bits_overwritten = 0;
        if (num_start_bits_overwritten < clot->length_in_bytes(parser_state) * 8) {
            for (auto slice : boost::adaptors::reverse(slices)) {
                LOG5("    Computing end bits overwritten For slice: " << slice);
                auto overwrite_mask = bits_overwritten<Endian::Little>(phv, clot, slice);
                int last_zero_idx = overwrite_mask.ffz(0);
                num_end_bits_overwritten += last_zero_idx;
                LOG5("      overwrite_mask: " << overwrite_mask
                        << ", last_zero_idx: " << last_zero_idx
                        << ", num_end_bits_overwritten: " << num_end_bits_overwritten);
                if (last_zero_idx < slice->size()) break;
            }
        }
        LOG5("  num_end_bits_overwritten:" << num_end_bits_overwritten
                << ", clot length_in_bytes: " << clot->length_in_bytes(parser_state));

        BUG_CHECK(num_end_bits_overwritten % 8 == 0,
                "CLOT %d ends with %d overwritten bits, which are not byte-aligned",
                clot->tag, num_end_bits_overwritten);

        crop(clot, parser_state, num_start_bits_overwritten, num_end_bits_overwritten);

        length_in_bytes += clot->length_in_bytes(parser_state);
    }

    return length_in_bytes > 0;
}

void ClotInfo::crop(Clot* clot, cstring parser_state, unsigned num_bits,
                    bool from_start) {
    Log::TempIndent indent;
    LOG3("Cropping clot for parser state " << parser_state << ", num_bits: " << num_bits
                                           << indent);
    if (num_bits == 0) return;

    unsigned num_bits_skipped = 0;
    BUG_CHECK(clot->parser_state_to_slices_.count(parser_state),
              "Tried to crop %1% in a parser state it is not present in (%2%)",
              *clot, parser_state);
    std::vector<const PHV::FieldSlice*> current_slices =
        clot->parser_state_to_slices().at(parser_state);
    if (!from_start) std::reverse(current_slices.begin(), current_slices.end());
    std::vector<const PHV::FieldSlice*> new_slices;
    for (auto slice : current_slices) {
        if (num_bits_skipped == num_bits) {
            new_slices.push_back(slice);
            continue;
        }

        auto field = slice->field();
        num_bits_skipped += slice->size();

        if (num_bits_skipped <= num_bits) {
            // Remove current slice.
            continue;
        }

        // Replace with a sub-slice of the current slice. We better not have a checksum field,
        // since we can only overwrite whole checksum fields.
        BUG_CHECK(!clot->is_checksum_field(field),
                  "Attempted to remove a slice of checksum field %s from CLOT %d",
                  field->name, clot->tag);

        auto new_size = num_bits_skipped - num_bits;

        auto cur_range = slice->range();
        auto shift_amt = from_start ? 0 : cur_range.size() - new_size;

        auto new_range = cur_range.shiftedByBits(shift_amt).resizedToBits(new_size);
        auto subslice = new PHV::FieldSlice(field, new_range);
        new_slices.push_back(subslice);

        num_bits_skipped = num_bits;
    }

    if (!from_start) std::reverse(new_slices.begin(), new_slices.end());
    LOG4("New slices: " << new_slices);
    clot->set_slices(parser_state, new_slices);
}

void ClotInfo::crop(Clot* clot, cstring parser_state, unsigned start_bits, unsigned end_bits) {
    BUG_CHECK(start_bits + end_bits <= clot->length_in_bytes(parser_state) * 8,
              "Cropping %d bits from CLOT %d, which is only %d bits long",
              start_bits + end_bits, clot->tag, clot->length_in_bytes(parser_state) * 8);

    crop(clot, parser_state, start_bits, true);
    crop(clot, parser_state, end_bits, false);
}

bool ClotInfo::slice_overwritten(const PhvInfo& phv,
                                 const Clot* clot,
                                 const PHV::FieldSlice* slice) const {
    return clot_covers_slice(clot, slice) && !bits_overwritten(phv, clot, slice).empty();
}

bool ClotInfo::slice_overwritten_by_phv(const PhvInfo& phv,
                                        const Clot* clot,
                                        const PHV::FieldSlice* slice) const {
    return clot_covers_slice(clot, slice) && !bits_overwritten_by_phv(phv, clot, slice).empty();
}

template <Endian Order>
bitvec ClotInfo::bits_overwritten(const PhvInfo& phv,
                                  const Clot* clot,
                                  const PHV::FieldSlice* slice) const {
    if (is_checksum(slice->field())) {
        LOG5("Field " << slice->field() << " is checksum");
        bitvec result;
        result.setrange(0, slice->size());
        return result;
    }

    LOG5("Field " << slice->field() << " is not checksum");
    return bits_overwritten_by_phv<Order>(phv, clot, slice);
}

template <Endian Order>
bitvec ClotInfo::bits_overwritten_by_phv(const PhvInfo& phv,
                                         const Clot* clot,
                                         const PHV::FieldSlice* slice) const {
    bitvec result;
    if (is_checksum(slice->field())) return result;
    Log::TempIndent indent;
    LOG5("Computing bits overwritten by PHV" << indent);

    const auto* field = slice->field();
    const auto& slice_range = slice->range();
    field->foreach_alloc(slice_range, PHV::AllocContext::DEPARSER, nullptr,
            [&](const PHV::AllocSlice& alloc) {
        // The container overwrites the CLOT if we were given a slice of a modified field.
        bool container_overwrites = is_modified(field);
        LOG6("Container overwrites: " << container_overwrites);

        // Handle other cases in which the container overwrites the CLOT.
        //
        // We can ignore any slice allocated to the container if that allocated slice satisfies any
        // of the following conditions:
        //
        //   - The given slice and the allocated slice are from mutually exclusive fields.
        //   - The given slice and the allocated slice from different fields, and those fields are
        //     allocated to overlapping ranges in the container.
        //
        // Otherwise, we know one of the following is true, in which case, the container overwrites
        // the CLOT if the CLOT does not completely cover the allocated slice.
        //
        //   - The given slice and the allocated slice are from the same field.
        //   - The given slice and the allocated slice are from different non-mutually-exclusive
        //     fields, and those fields are allocated to non-overlapping ranges in the container.
        if (!container_overwrites) {
            // Get the current container and figure out which bits of the container are occupied by
            // the field of the slice we were given.
            auto container = alloc.container();
            auto occupied_bits =
                phv.bits_allocated(container, field, PHV::AllocContext::DEPARSER);
            LOG6("Container occupied_bits: " << occupied_bits);

            // Go through the analysis described above.
            PHV::FieldUse use(PHV::FieldUse::READ);
            for (const auto& alloc_slice :
                 phv.get_slices_in_container(container, PHV::AllocContext::DEPARSER, &use)) {
                const auto* other_field = alloc_slice.field();
                LOG7("  Container read slice: " << alloc_slice << ", other field: " << other_field);

                // Ignore if field and other_field are mutually exclusive.
                if (phv.isFieldMutex(field, other_field)) continue;
                LOG7("    Fields are not mutex");

                // Ignore if field and other_field are different, and alloc_slice overlaps with
                // occupied_bits in the container.
                auto other_occupied =
                    phv.bits_allocated(container, other_field, PHV::AllocContext::DEPARSER);
                LOG7("    Other field occupied bits: " << other_occupied);
                if (field != other_field && !(occupied_bits & other_occupied).empty()) continue;
                LOG7("    Different fields and no overlap");

                // Container overwrites the CLOT if the CLOT doesn't completely cover the allocated
                // slice or if the other slice is modified.
                const auto* other_slice = new PHV::FieldSlice(other_field,
                                                              alloc_slice.field_slice());
                container_overwrites =
                    !clot_covers_slice(clot, other_slice) || is_modified(other_field);
                LOG7("    other slice: " << other_slice
                        << ", container_overwrites: " << container_overwrites
                        << ", is_modified: " << is_modified(other_field)
                        << ", clot_covers_slice: " << clot_covers_slice(clot, other_slice));
                if (container_overwrites) break;
            }
        }

        if (!container_overwrites) return;

        auto overwrite_range = alloc.field_slice().intersectWith(slice_range);

        overwrite_range = overwrite_range.shiftedByBits(-slice_range.lo);
        auto normalized_overwrite_range = overwrite_range.toOrder<Order>(slice_range.size());

        result.setrange(normalized_overwrite_range.lo, normalized_overwrite_range.size());
    });

    return result;
}

bool ClotInfo::is_added_by_mau(cstring h) const {
    return headers_added_by_mau_.count(h);
}

assoc::map<const PHV::FieldSlice*, Clot*, PHV::FieldSlice::Greater>*
ClotInfo::slice_clots(const PHV::FieldSlice* slice) const {
    auto result = new assoc::map<const PHV::FieldSlice*, Clot*, PHV::FieldSlice::Greater>();
    auto field = slice->field();
    for (auto clot : clots_) {
        const auto& fields_to_slices = clot->fields_to_slices();
        if (!fields_to_slices.count(field)) continue;

        auto clot_slice = fields_to_slices.at(field);
        if (clot_slice->range().overlaps(slice->range()))
            (*result)[clot_slice] = clot;
    }

    return result;
}

Clot* ClotInfo::whole_field_clot(const PHV::Field* field) const {
    for (auto clot : clots_)
        if (clot->has_slice(new PHV::FieldSlice(field)))
            return clot;
    return nullptr;
}

bool ClotInfo::clot_covers_slice(const Clot* clot, const PHV::FieldSlice* slice) const {
    const auto& fields_to_slices = clot->fields_to_slices();
    auto field = slice->field();
    for (auto fs : fields_to_slices) {
        LOG5("Fields to slices " << fs.first << " : " << fs.second);
    }
    if (!fields_to_slices.count(field)) return false;

    auto clot_slice = fields_to_slices.at(field);
    LOG5("Clot slice " << clot_slice << ", slice: " << slice);
    return clot_slice->range().contains(slice->range());
}

std::string ClotInfo::print(const PhvInfo* phvInfo) const {
    std::stringstream out;

    unsigned total_unused_fields_in_clots = 0;
    unsigned total_unused_bits_in_clots = 0;

    std::set<int> unaligned_clots;

    out << std::endl;
    for (auto gress : {INGRESS, EGRESS}) {
        unsigned total_bits = 0;
        out << "CLOT Allocation (" << toString(gress) << "):" << std::endl;
        TablePrinter tp(out, {"CLOT", "Fields", "Bits", "Property"},
                              TablePrinter::Align::CENTER);

        for (auto entry : clot_to_parser_states()) {
            auto* clot = entry.first;
            auto& pair = entry.second;
            if (gress != pair.first) continue;

            unsigned bits_in_clot = 0;

            std::set<const PHV::FieldSlice*> counted_slices;
            const auto state_to_slices = clot->parser_state_to_slices();
            for (auto it = state_to_slices.begin(); it != state_to_slices.end(); ++it) {
                bool first_row = (it == state_to_slices.begin());
                cstring parser_state = it->first;

                tp.addRow({first_row ? std::to_string(clot->tag) : "",
                "state " + std::string(parser_state),
                std::to_string(clot->length_in_bytes(parser_state)) + " bytes",
                ""});
                tp.addBlank();

                std::vector<const PHV::FieldSlice*> slices = it->second;
                for (const auto* slice : slices) {
                    if (counted_slices.insert(slice).second && is_unused(slice)) {
                        total_unused_fields_in_clots++;
                        total_unused_bits_in_clots += slice->size();
                    }

                    std::stringstream bits;
                    bits << slice->size()
                         << " [" << clot->bit_offset(parser_state, slice) << ".."
                         << (clot->bit_offset(parser_state, slice) + slice->size() - 1)
                         << "]";

                    bool is_phv = clot->is_phv_field(slice->field());
                    bool is_checksum = clot->is_checksum_field(slice->field());
                    bool is_phv_overwrite = phvInfo &&
                        slice_overwritten_by_phv(*phvInfo, clot, slice);

                    std::string attr;
                    if (is_phv || is_checksum) {
                        attr += " (";
                        if (is_phv) attr += " phv";
                        if (is_phv_overwrite) attr += "*";
                        if (is_checksum) attr += " csum";
                        attr += " ) ";
                    }

                    tp.addRow({"", std::string(slice->shortString()), bits.str(), attr});
                    bits_in_clot = std::max(bits_in_clot,
                        clot->bit_offset(parser_state, slice) + slice->size());
                }
                bool last_row = (std::next(it) == state_to_slices.end());
                if (!last_row)
                    tp.addBlank();
            }
            total_bits += bits_in_clot;
            if (bits_in_clot % 8 != 0)
                unaligned_clots.insert(clot->tag);

            tp.addSep();
        }

        tp.addRow({"", "Total Bits", std::to_string(total_bits), ""});
        tp.print();
        out << std::endl;

        // Also report the maximum number of CLOTs a packet will use.
        for (auto& kv : parserInfo.graphs()) {
            auto& graph = kv.second;
            if (graph->root->gress != gress) continue;

            auto state_to_clots = parser_state_to_clots(gress);
            auto largest = find_largest_paths(state_to_clots, graph, graph->root);
            out << "  Packets will use up to " << largest->first << " CLOTs." << std::endl;
            out << "  The parser path(s) that will use the most CLOTs contain the following "
                << "states:" << std::endl;
            for (auto state : *largest->second) {
                int num_clots_in_state = 0;
                auto state_name = sanitize_state_name(state->name, state->thread());
                if (state_to_clots.count(state_name))
                    num_clots_in_state = state_to_clots.at(state_name).size();
                out << "    " << state->name << " (" << num_clots_in_state << " CLOTs)"
                    << std::endl;
            }
            out << std::endl;
        }
    }

    unsigned total_unused_fields = 0;
    unsigned total_unused_bits = 0;

    out << "All fields:" << std::endl;

    TablePrinter field_tp(out, {"Field", "Bits", "CLOTs", "Property"},
                               TablePrinter::Align::CENTER);
    for (auto kv : field_to_parser_states_) {
        auto& field = kv.first;
        auto& states_to_extracts = kv.second;

        // Ignore extracts that aren't always sourced from the packet.
        bool pkt_src = true;
        for (auto& kv2 : states_to_extracts) {
            auto& state = kv2.first;
            pkt_src = field_range(state, field) != std::nullopt;
            if (!pkt_src) break;
        }
        if (!pkt_src) continue;

        std::string attr;
        if (is_checksum(field)) {
            attr = "checksum";
        } else if (is_modified(field)) {
            attr = "modified";
        } else if (is_readonly(field)) {
            attr = "read-only";
        } else {
            attr = "unused";
            total_unused_fields++;
            total_unused_bits += field->size;
        }

        std::stringstream tags;
        bool first_clot = true;
        for (auto entry : *slice_clots(field)) {
            if (!first_clot) tags << ", ";
            tags << entry.second->tag;
            first_clot = false;
        }

        field_tp.addRow({std::string(field->name), std::to_string(field->size), tags.str(), attr});
    }

    field_tp.addSep();
    field_tp.addRow({"Unused fields", std::to_string(total_unused_fields), "", ""});
    field_tp.addRow({"Unused bits", std::to_string(total_unused_bits), "", ""});
    field_tp.addRow({"Unused CLOT-allocated fields",
                     std::to_string(total_unused_fields_in_clots), "", ""});
    field_tp.addRow({"Unused CLOT-allocated bits",
                     std::to_string(total_unused_bits_in_clots), "", ""});

    field_tp.print();

    // Bug-check.
    if (!unaligned_clots.empty()) {
        std::clog << out.str();
        std::stringstream out;
        bool first_tag = true;
        int count = 0;
        for (auto tag : unaligned_clots) {
            out << (first_tag ? "" : ", ") << tag;
            first_tag = false;
            count++;
        }
        BUG("CLOT%s %s not byte-aligned", count > 1 ? "s" : "", out.str());
    }

    return out.str();
}

void ClotInfo::add_alias(const PHV::Field* f1, const PHV::Field* f2) {
    std::set<const PHV::Field*>* f1_aliases =
        field_aliases_.count(f1) ? field_aliases_.at(f1) : nullptr;
    std::set<const PHV::Field*>* f2_aliases =
        field_aliases_.count(f2) ? field_aliases_.at(f2) : nullptr;

    if (f1_aliases == nullptr && f2_aliases == nullptr) {
        auto aliases = new std::set<const PHV::Field*>();
        aliases->insert(f1);
        aliases->insert(f2);
        field_aliases_[f1] = field_aliases_[f2] = aliases;
    } else if (f1_aliases != nullptr && f2_aliases != nullptr) {
        // Merge the two equivalence classes.
        for (auto field : *f2_aliases) {
            f1_aliases->insert(field);
            field_aliases_[field] = f1_aliases;
        }
    } else if (f1_aliases != nullptr) {
        f1_aliases->insert(f2);
        field_aliases_[f2] = f1_aliases;
    } else {
        f2_aliases->insert(f1);
        field_aliases_[f1] = f2_aliases;
    }
}

void ClotInfo::clear() {
    is_modified_.clear();
    do_not_use_clot_fields.clear();
    parser_state_to_fields_.clear();
    parser_state_to_header_stacks_.clear();
    header_stack_elements_.clear();
    field_to_parser_states_.clear();
    fields_to_pov_bits_.clear();
    field_to_byte_idx.clear();
    byte_idx_to_field.clear();
    checksum_dests_.clear();
    field_to_checksum_updates_.clear();
    clots_.clear();
    clot_to_parser_states_.clear();
    parser_state_to_clots_[INGRESS].clear();
    parser_state_to_clots_[EGRESS].clear();
    field_range_.clear();
    field_aliases_.clear();
    headers_added_by_mau_.clear();
    field_to_pseudoheaders_.clear();
    deparse_graph_.clear();
    Clot::tag_count.clear();
    Pseudoheader::nextId = 0;
}

std::pair<unsigned, ordered_set<const IR::BFN::ParserState*>*>* ClotInfo::find_largest_paths(
        const std::map<cstring, std::set<const Clot*>>& parser_state_to_clots,
        const IR::BFN::ParserGraph* graph,
        const IR::BFN::ParserState* state,
        std::map<const IR::BFN::ParserState*,
                 std::pair<unsigned,
                           ordered_set<const IR::BFN::ParserState*>*>*>* memo /*= nullptr*/) const {
    // Create the memoization table if needed, and make sure we haven't visited the state already.
    if (memo == nullptr) {
        memo = new std::map<const IR::BFN::ParserState*,
                             std::pair<unsigned, ordered_set<const IR::BFN::ParserState*>*>*>();
    } else if (memo->count(state)) {
        return memo->at(state);
    }

    // Recursively find the largest paths of the children, and aggregate the results.
    unsigned max_clots = 0;
    auto max_path_states = new ordered_set<const IR::BFN::ParserState*>();
    if (graph->successors().count(state)) {
        for (auto child : graph->successors().at(state)) {
            const auto result = find_largest_paths(parser_state_to_clots, graph, child, memo);
            if (result->first > max_clots) {
                max_clots = result->first;
                max_path_states->clear();
            }

            if (result->first == max_clots) {
                max_path_states->insert(result->second->begin(), result->second->end());
            }
        }
    }

    // Add the current state's result to the table and return.
    auto state_name = sanitize_state_name(state->name, state->thread());
    if (parser_state_to_clots.count(state_name))
        max_clots += parser_state_to_clots.at(state_name).size();
    max_path_states->insert(state);
    auto result =
        new std::pair<unsigned, ordered_set<const IR::BFN::ParserState*>*>(
            max_clots, max_path_states);
    return (*memo)[state] = result;
}

Visitor::profile_t CollectClotInfo::init_apply(const IR::Node* root) {
    auto rv = Inspector::init_apply(root);
    clotInfo.clear();
    clotInfo.do_not_use_clot_fields = pragmaDoNotUseClot.do_not_use_clot_fields();

    // Configure logging for this visitor.
    if (BackendOptions().verbose > 0) {
        if (auto pipe = root->to<IR::BFN::Pipe>())
            log = new Logging::FileLog(pipe->canon_id(), "clot_allocation.log"_cs);
    }

    return rv;
}

bool CollectClotInfo::preorder(const IR::BFN::Extract* extract) {
    auto state = findContext<IR::BFN::ParserState>();

    if (auto field_lval = extract->dest->to<IR::BFN::FieldLVal>()) {
        if (auto f = phv.field(field_lval->field)) {
            clotInfo.add_field(f, extract->source, state);
        }
    }

    return true;
}

bool CollectClotInfo::preorder(const IR::BFN::ParserZeroInit* zero_init) {
    auto state = findContext<IR::BFN::ParserState>();

    if (auto field_lval = zero_init->field->to<IR::BFN::FieldLVal>()) {
        if (auto f = phv.field(field_lval->field)) {
            clotInfo.add_field(f, new IR::BFN::ConstantRVal(0), state);
        }
    }

    return true;
}

bool CollectClotInfo::preorder(const IR::HeaderStackItemRef* hsir) {
    auto state = findContext<IR::BFN::ParserState>();
    if (state) {
        const auto* base = hsir->baseRef();
        if (!base) return true;
        bool in_loopback = clotInfo.parserInfo.graph(state).is_loopback_state(state->name);
        if (!in_loopback) {
            for (auto& [states, _] : clotInfo.parserInfo.graph(state).loopbacks()) {
                if (states.first == state) {
                    in_loopback = true;
                    break;
                }
            }
        }
        if (in_loopback) {
            if (const auto* hs = base->to<IR::HeaderStack>()) {
                int index = -1;
                if (const auto* c = hsir->index()->to<IR::Constant>())
                    index = c->asInt();
                clotInfo.add_stack(hs, state, index);
            }
        }
    }

    return true;
}

bool CollectClotInfo::preorder(const IR::BFN::EmitField* emit) {
    auto field = phv.field(emit->source->field);
    auto irPov = emit->povBit->field;

    le_bitrange slice;
    const PHV::Field* pov;
    if (auto alias_slice = irPov->to<IR::BFN::AliasSlice>())
        pov = phv.field(alias_slice->source, &slice);
    else
        pov = phv.field(irPov, &slice);

    clotInfo.fields_to_pov_bits_[field].insert(new PHV::FieldSlice(pov, slice));
    return true;
}

bool CollectClotInfo::preorder(const IR::BFN::EmitChecksum* emit) {
    auto f = phv.field(emit->dest->field);
    clotInfo.checksum_dests_.insert(f);

    for (auto s : emit->sources) {
        auto src = phv.field(s->field->field);
        clotInfo.field_to_checksum_updates_[src].push_back(emit);
    }

    le_bitrange slice;
    const PHV::Field* pov;
    if (auto alias_slice = emit->povBit->field->to<IR::BFN::AliasSlice>())
        pov = phv.field(alias_slice->source, &slice);
    else
        pov = phv.field(emit->povBit->field, &slice);

    clotInfo.fields_to_pov_bits_[f].insert(new PHV::FieldSlice(pov, slice));

    return true;
}

void CollectClotInfo::postorder(const IR::BFN::Deparser* deparser) {
    // Used for deduplicating pseudoheaders. Contains the POV bits and fields of pseudoheaders
    // that have already been allocated, mapped to the allocated pseudoheader.
    std::map<std::pair<const PovBitSet,
                       const std::vector<const PHV::Field*>>,
             const Pseudoheader*> allocated;

    // The POV bits and field list for the current pseudoheader we are building.
    PovBitSet cur_pov_bits;
    std::vector<const PHV::Field*> cur_fields;

    // Tracks the graph node corresponding to the field or constant that was previously emitted by
    // the deparser.
    std::optional<DeparseGraph::Node> prev_node = std::nullopt;

    // The deparse graph for the current gress.
    auto& deparse_graph = clotInfo.deparse_graph_[deparser->gress];

    // Checksum updates (emits) for the current block of fields if any
    std::vector<const IR::BFN::EmitChecksum*> cur_checksum_updates;

    auto get_checksum_updates = [&](const PHV::Field* field) {
        return clotInfo.field_to_checksum_updates().count(field) ?
            clotInfo.field_to_checksum_updates().at(field) :
            std::vector<const IR::BFN::EmitChecksum*>();
    };

    for (auto emit : deparser->emits) {
        const PHV::Field* cur_field = nullptr;
        DeparseGraph::Node cur_node;

        if (auto emit_field = emit->to<IR::BFN::EmitField>()) {
            cur_field = phv.field(emit_field->source->field);
            cur_node = deparse_graph.addField(cur_field);
        } else if (auto emit_checksum = emit->to<IR::BFN::EmitChecksum>()) {
            cur_field = phv.field(emit_checksum->dest->field);
            cur_node = deparse_graph.addField(cur_field);
        } else if (auto emit_constant = emit->to<IR::BFN::EmitConstant>()) {
            // exclude from pseudoheader (below)
            // can potentially combine constants and fields with same
            // pov bit sets to create bigger pseudoheader? TODO
            cur_node = deparse_graph.addConst(emit_constant->constant);
        } else {
            BUG("Unexpected deparser emit: %1%", emit);
        }

        if (!cur_field) {
            // Current emit is a constant. Create a pseudoheader with current set of fields.
            add_pseudoheader(cur_pov_bits, cur_fields, allocated);
            cur_pov_bits.clear();
            cur_fields.clear();
            cur_checksum_updates.clear();
        } else {
            // Current emit is a field. Create a pseudoheader if its pov set is different from
            // previous field or checksum updates is different from previous (if they have updates)
            auto pov_bits = clotInfo.fields_to_pov_bits_.at(cur_field);
            auto checksum_updates = get_checksum_updates(cur_field);

            if (cur_pov_bits != pov_bits ||
                (!cur_checksum_updates.empty() && !checksum_updates.empty() &&
                 cur_checksum_updates != checksum_updates)) {
                add_pseudoheader(cur_pov_bits, cur_fields, allocated);
                cur_pov_bits = pov_bits;
                cur_fields.clear();
                cur_checksum_updates.clear();
            }

            cur_fields.push_back(cur_field);
            if (!checksum_updates.empty()) cur_checksum_updates = checksum_updates;
        }

        // Update the deparse graph with the current field and set things up for the next
        // iteration.
        if (prev_node) {
            deparse_graph.addEdge(*prev_node, cur_node);
        }
        prev_node = cur_node;
    }

    add_pseudoheader(cur_pov_bits, cur_fields, allocated);
}

void CollectClotInfo::add_pseudoheader(
    const PovBitSet pov_bits,
    const std::vector<const PHV::Field*> fields,
    std::map<std::pair<const PovBitSet,
                       const std::vector<const PHV::Field*>>,
             const Pseudoheader*>& allocated
) {
    if (fields.empty()) return;

    auto key = std::make_pair(pov_bits, fields);
    if (allocated.count(key)) return;

    // Have a new pseudoheader.
    auto pseudoheader = new Pseudoheader(pov_bits, fields);
    for (auto field : fields) {
        clotInfo.field_to_pseudoheaders_[field].insert(pseudoheader);
    }

    allocated[key] = pseudoheader;

    if (LOGGING(6)) {
        LOG6("Pseudoheader " << pseudoheader->id);
        std::stringstream out;
        out << "  POV bits: ";
        bool first = true;
        for (auto pov : pov_bits) {
            if (!first) out << ", ";
            first = false;
            out << pov->shortString();
        }
        LOG6(out.str());
        LOG6("  Fields:");
        for (auto field : fields)
            LOG6("    " << field->name);
    }
}

void CollectClotInfo::add_alias_field(const IR::Expression* alias) {
    const PHV::Field* aliasSource = nullptr;
    const PHV::Field* aliasDestination = nullptr;
    if (auto aliasMem = alias->to<IR::BFN::AliasMember>()) {
        aliasSource = phv.field(aliasMem->source);
        aliasDestination = phv.field(aliasMem);
    } else if (auto aliasSlice = alias->to<IR::BFN::AliasSlice>()) {
        aliasSource = phv.field(aliasSlice->source);
        aliasDestination = phv.field(aliasSlice);
    }
    BUG_CHECK(aliasSource, "Alias source for field %1% not found", alias);
    BUG_CHECK(aliasDestination, "Reference to alias field %1% not found", alias);
    clotInfo.add_alias(aliasSource, aliasDestination);
}

bool CollectClotInfo::preorder(const IR::MAU::Instruction* instruction) {
    // Make sure we have a call to "set".
    if (instruction->name != "set") return true;

    auto dst = instruction->operands.at(0);
    auto src = instruction->operands.at(1);

    // Make sure we are setting a header's validity bit.
    le_bitrange bitrange;
    auto dst_field = phv.field(dst, &bitrange);
    if (!dst_field || !dst_field->pov) return true;

    // Handle case where we are assigning a zero constant to the validity bit. Conservatively, we
    // assume that assigning a non-constant value to the POV bit can change its value arbitrarily.
    if (auto constant = src->to<IR::Constant>()) {
        if (constant->value == 0) return true;
    }

    // If we have a header stack then mark elements of the stack as added
    auto *stacks = findContext<IR::BFN::Pipe>()->headerStackInfo;
    if (dst_field->name.endsWith("$stkvalid") && stacks->count(dst_field->header())) {
        // Can we grab the element name from an AliasSource?
        const IR::BFN::AliasSlice* as = dst->to<IR::BFN::AliasSlice>();
        const PHV::Field* alias_field;
        if (as && (alias_field = phv.field(as->source, &bitrange))) {
            clotInfo.headers_added_by_mau_.insert(alias_field->header());
        } else {
            // Couldn't identify the true source, so mark all elems in the hdr stack as added
            auto& stack = stacks->at(dst_field->header());
            for (int i = 0; i < stack.size; ++i) {
                cstring elem = dst_field->header() + '[' + std::to_string(i).c_str() + ']';
                clotInfo.headers_added_by_mau_.insert(elem);
            }
        }
    } else {
        clotInfo.headers_added_by_mau_.insert(dst_field->header());
    }

    return true;
}

void CollectClotInfo::end_apply(const IR::Node* root) {
    clotInfo.compute_byte_maps();
    Logging::FileLog::close(log);
    Inspector::end_apply(root);
}

#if BAREFOOT_INTERNAL
void dump(std::ostream &out, const Clot &clot) {
    for (auto &fs : clot.fields_to_slices())
        out << fs.first->name << ": " << *fs.second << std::endl;
}
void dump(const Clot &clot) { dump(std::cout, clot); }
void dump(const Clot *clot) { dump(std::cout, *clot); }

void dump(const ClotInfo &ci) { std::cout << ci.print(); }
void dump(const ClotInfo *ci) { std::cout << ci->print(); }
#endif
