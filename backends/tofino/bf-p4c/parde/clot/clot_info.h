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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CLOT_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CLOT_INFO_H_

#include <algorithm>

#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/lib/cmp.h"
#include "bf-p4c/logging/filelog.h"
#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "clot.h"
#include "deparse_graph.h"
#include "field_slice_set.h"
#include "lib/ordered_map.h"
#include "pseudoheader.h"

class PhvInfo;

class FieldSliceExtractInfo;

class ClotInfo {
    friend class AllocateClot;
    friend class CollectClotInfo;
    friend class ClotCandidate;
    friend class GreedyClotAllocator;

    PhvUse &uses;

    /// Fields that are ineligible for CLOTs due to pragma do_not_use_clot
    ordered_set<const PHV::Field *> do_not_use_clot_fields;

    /// Maps parser states to all fields extracted in that state, regardless of source.
    ordered_map<const IR::BFN::ParserState *, std::vector<const PHV::Field *>>
        parser_state_to_fields_;

    /// Maps parser states to all header stacks extracted in that state if the
    /// state is a loop target
    ordered_map<const IR::BFN::ParserState *, std::vector<const IR::HeaderStack *>>
        parser_state_to_header_stacks_;

    /// Maps parser states + header header state to elements seen
    ordered_map<std::pair<const IR::BFN::ParserState *, const IR::HeaderStack *>,
                std::set<unsigned>>
        header_stack_elements_;

    /// Maps fields to all states that extract the field, regardless of source. Each state is
    /// further mapped to the source from which the field is extracted.
    ordered_map<const PHV::Field *,
                std::unordered_map<const IR::BFN::ParserState *, const IR::BFN::ParserRVal *>>
        field_to_parser_states_;

    /// Maps the fields extracted from the packet in each state to the state-relative byte indices
    /// occupied by the field.
    ordered_map<const IR::BFN::ParserState *, ordered_map<const PHV::Field *, std::set<unsigned>>>
        field_to_byte_idx;

    /// Maps state-relative byte indices in each state to the fields occupying
    /// that byte.
    ordered_map<const IR::BFN::ParserState *, std::map<unsigned, ordered_set<const PHV::Field *>>>
        byte_idx_to_field;

    std::set<const PHV::Field *> checksum_dests_;
    std::map<const PHV::Field *, std::vector<const IR::BFN::EmitChecksum *>>
        field_to_checksum_updates_;

    std::vector<Clot *> clots_;
    std::map<const Clot *, std::pair<gress_t, std::set<cstring>>, Clot::Less>
        clot_to_parser_states_;
    /// Maps states in gress to set of extracted Clots
    /// When looking up in the inner map, do use sanitize_state_name on the lookup key
    std::map<gress_t, std::map<cstring, std::set<const Clot *>>> parser_state_to_clots_;

    std::map<const IR::BFN::ParserState *, ordered_map<const PHV::Field *, nw_bitrange>>
        field_range_;

    std::map<const Clot *, std::vector<const IR::BFN::EmitChecksum *>> clot_to_emit_checksum_;

    /// Maps fields to their equivalence class of aliases.
    std::map<const PHV::Field *, std::set<const PHV::Field *> *> field_aliases_;

    /// Names of headers that might be added by MAU.
    std::set<cstring> headers_added_by_mau_;

    /// Maps each field that is part of a pseudoheader to the pseudoheaders containing that field.
    std::map<const PHV::Field *, std::set<const Pseudoheader *>> field_to_pseudoheaders_;

    std::map<gress_t, DeparseGraph> deparse_graph_;

    /// Maps each field to its pov bits.
    std::map<const PHV::Field *, PovBitSet> fields_to_pov_bits_;

 public:
    explicit ClotInfo(PhvUse &uses) : uses(uses) {}

 private:
    unsigned num_clots_allocated(gress_t gress) const { return Clot::tag_count.at(gress); }

    /// The extracted packet range of a given field for a given parser state.
    std::optional<nw_bitrange> field_range(const IR::BFN::ParserState *state,
                                           const PHV::Field *field) const;

    /// The bit offset of a given field for a given parser state.
    std::optional<unsigned> offset(const IR::BFN::ParserState *state,
                                   const PHV::Field *field) const;

 public:
    /// Sanitizes state name before lookup in the parser_state_to_clots_
    ///
    /// If state name is not prefixed with gress, then it will be prefixed
    /// If a state is compiler generated auxiliary state (thus containing
    /// '.$' substring), then the generated appendix is stripped to
    /// regenerate original state name.
    cstring sanitize_state_name(cstring state_name, gress_t gress) const;

    CollectParserInfo parserInfo;

    const ordered_map<const IR::BFN::ParserState *, std::vector<const PHV::Field *>> &
    parser_state_to_fields() const {
        return parser_state_to_fields_;
    };

    const ordered_map<const IR::BFN::ParserState *, std::vector<const IR::HeaderStack *>> &
    parser_state_to_header_stacks() const {
        return parser_state_to_header_stacks_;
    };

    const ordered_map<std::pair<const IR::BFN::ParserState *, const IR::HeaderStack *>,
                      std::set<unsigned>> &
    header_stack_elements() {
        return header_stack_elements_;
    }

    /// A POV is in this set if there is a path through the parser in which the field is not
    /// extracted, but its POV bit is set.
    std::set<const PHV::Field *> pov_extracted_without_fields;

    const std::map<const Clot *, std::pair<gress_t, std::set<cstring>>, Clot::Less> &
    clot_to_parser_states() const {
        return clot_to_parser_states_;
    }

    std::map<cstring, std::set<const Clot *>> &parser_state_to_clots(gress_t gress) {
        return parser_state_to_clots_.at(gress);
    }

    const std::map<cstring, std::set<const Clot *>> &parser_state_to_clots(gress_t gress) const {
        return parser_state_to_clots_.at(gress);
    }

    const std::set<const Clot *> parser_state_to_clots(
        const IR::BFN::LoweredParserState *state) const;

    const Clot *parser_state_to_clot(const IR::BFN::LoweredParserState *state, unsigned tag) const;

    const std::vector<Clot *> &clots() { return clots_; }

    const std::map<const PHV::Field *, std::vector<const IR::BFN::EmitChecksum *>> &
    field_to_checksum_updates() {
        return field_to_checksum_updates_;
    }

    /// @return a map from overwrite offsets to corresponding containers.
    std::map<int, PHV::Container> get_overwrite_containers(const Clot *clot,
                                                           const PhvInfo &phv) const;

    /// @return a map from overwrite offsets to corresponding checksum fields.
    std::map<int, const PHV::Field *> get_csum_fields(const Clot *clot) const;

    std::map<const Clot *, std::vector<const IR::BFN::EmitChecksum *>> &clot_to_emit_checksum() {
        return clot_to_emit_checksum_;
    }

    const std::map<const Clot *, std::vector<const IR::BFN::EmitChecksum *>> &
    clot_to_emit_checksum() const {
        return clot_to_emit_checksum_;
    }

    /// If gress::src_state_name state has CLOT extracts, then add them as gress::dst_state_name
    /// extracts as well.
    ///
    /// This is because MergeLoweredParserStates merges LoweredParser states, their CLOT extracts
    /// need to be merged accordingly so ClotResourcesLogging has correct information
    /// for logging.
    void merge_parser_states(gress_t gress, cstring dst_state_name, cstring src_state_name);

 private:
    void add_field(const PHV::Field *f, const IR::BFN::ParserRVal *source,
                   const IR::BFN::ParserState *state);

    void add_stack(const IR::HeaderStack *hs, const IR::BFN::ParserState *state, int index = -1);

    /// Populates @ref field_to_byte_idx and @ref byte_idx_to_field.
    void compute_byte_maps();

    void add_clot(Clot *clot, ordered_set<const IR::BFN::ParserState *> states);

    /// @return a set of parser states to which no more CLOTs may be allocated,
    /// because doing so would exceed the maximum number CLOTs allowed per packet.
    const ordered_set<const IR::BFN::ParserState *> *find_full_states(
        const IR::BFN::ParserGraph *graph) const;

    // A field may participate in multiple checksum updates, e.g. IPv4 fields
    // may be included in both IPv4 and TCP checksum updates. In such cases,
    // we require the IPv4 fields in both updates to be identical sets in order
    // to be allocated to a CLOT (each CLOT can only compute one checksum)
    bool is_used_in_multiple_checksum_update_sets(const PHV::Field *field) const;

    /// Determines whether a field is extracted in multiple states that are not mutually exclusive.
    bool is_extracted_in_multiple_non_mutex_states(const PHV::Field *f) const;

    /// Returns true when all paths that set the POV bit of @p field also extracts @p field.
    bool extracted_with_pov(const PHV::Field *field) const;

    /// Determines whether a field has the same bit-in-byte offset (i.e., the same bit offset,
    /// modulo 8) in all parser states that extract the field.
    bool has_consistent_bit_in_byte_offset(const PHV::Field *field) const;

    /// Determines whether a field can be part of a CLOT. This can include
    /// fields that are PHV-allocated.
    bool can_be_in_clot(const PHV::Field *field) const;

    /// Determines whether a field slice can be the first one in a CLOT.
    bool can_start_clot(const FieldSliceExtractInfo *extract_info) const;

    /// Determines whether a field slice can be the last one in a CLOT.
    bool can_end_clot(const FieldSliceExtractInfo *extract_info) const;

    /// Determines whether a field extracts its full width whenever it is extracted from the
    /// packet. Returns false if, in any state, the field is not extracted from the packet or is
    /// extracted from a non-packet source.
    bool extracts_full_width(const PHV::Field *field) const;

    /// Memoization table for @ref is_modified.
    mutable std::map<const PHV::Field *, bool> is_modified_;

 public:
    /// Determines whether a field is a checksum field.
    bool is_checksum(const PHV::Field *field) const;
    bool is_checksum(const PHV::FieldSlice *slice) const;

    /// Determines whether a field is modified, as defined in
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    bool is_modified(const PHV::Field *field) const;
    bool is_modified(const PHV::FieldSlice *slice) const;

    /// Determines whether a field is read-only, as defined in
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    bool is_readonly(const PHV::Field *field) const;
    bool is_readonly(const PHV::FieldSlice *slice) const;

    /// Determines whether a field is unused, as defined in
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    bool is_unused(const PHV::Field *field) const;
    bool is_unused(const PHV::FieldSlice *slice) const;

    /// Produces the set of CLOT-eligible fields.
    const std::set<const PHV::Field *> *clot_eligible_fields() const;

    /// @return nullptr if the @p field is read-only or modified. Otherwise, if the @p field is
    /// unused, returns a map from each CLOT-allocated slice for the field to its corresponding
    /// CLOT.
    assoc::map<const PHV::FieldSlice *, Clot *, PHV::FieldSlice::Greater> *allocated_slices(
        const PHV::Field *field) const {
        return is_unused(field) ? slice_clots(field) : nullptr;
    }

    /// Returns false if packet offset of the slice is less than hdr_len_adj and if the slice
    /// cannot be trimmed to increase its offset.
    bool is_slice_below_min_offset(const PHV::FieldSlice *slice, int max_packet_bit_offset) const;

    /// @return nullptr if the field containing the @p slice is read-only or modified. Otherwise,
    /// if the field containing the @p slice is unused, returns each CLOT-allocated slice that
    /// overlaps with the given @p slice, mapped to its corresponding CLOT.
    //
    // If we had more precise slice-level usage information, we could instead return a non-null
    // result if the given slice were unused.
    assoc::map<const PHV::FieldSlice *, Clot *, PHV::FieldSlice::Greater> *allocated_slices(
        const PHV::FieldSlice *slice) const {
        return is_unused(slice->field()) ? slice_clots(slice) : nullptr;
    }

    /// @return the given field's CLOT if the field is unused and is entirely covered in a CLOT.
    /// Otherwise, nullptr is returned.
    const Clot *fully_allocated(const PHV::Field *field) const {
        return is_unused(field) ? whole_field_clot(field) : nullptr;
    }

    /// @return the given field slice's CLOT allocation if the field containing the slice is unused
    /// and the slice is entirely CLOT-allocated. Otherwise, nullptr is returned.
    //
    // If we had more precise slice-level usage information, we could instead return a non-null
    // result if the given slice were unused.
    std::set<const Clot *, Clot::Less> *fully_allocated(const PHV::FieldSlice &slice) const {
        return fully_allocated(&slice);
    }

    /// @return the given field slice's CLOT allocation if the field containing the slice is unused
    /// and the slice is entirely CLOT-allocated. Otherwise, nullptr is returned.
    //
    // If we had more precise slice-level usage information, we could instead return a non-null
    // result if the given slice were unused.
    std::set<const Clot *, Clot::Less> *fully_allocated(const PHV::FieldSlice *slice) const;

    /// @return true if @p field is (1) fully allocated to clot, (2) not modified by MAUs, and
    /// (3) not digested. For these fields, it is safe to ignore deparser read to shrink their
    /// live ranges, because deparser can emit them directly from clot.
    bool allocated_unmodified_undigested(const PHV::Field *field) const {
        return whole_field_clot(field) && (is_readonly(field) || is_unused(field)) &&
               !field->is_digest();
    }

    /// Adjusts all allocated CLOTs so that they neither start nor end with an overwritten field
    /// slice. See \ref clot_alloc_adjust "CLOT allocation adjustment" (README.md).
    void adjust_clots(const PhvInfo &phv);

 private:
    /// Adjusts a CLOT so that it neither starts nor ends with an overwritten field slice. See
    /// \ref clot_alloc_adjust "CLOT allocation adjustment" (README.md).
    ///
    /// @return true if the CLOT is non-empty as a result of the adjustment.
    bool adjust(const PhvInfo &phv, Clot *clot);

    /// Removes bits from the start and end of the given @p clot.
    void crop(Clot *clot, cstring parser_state, unsigned start_bits, unsigned end_bits);

    void crop(Clot *clot, cstring parser_state, unsigned num_bits, bool from_start);

 public:
    /// Determines whether a field slice in a CLOT will be overwritten by a PHV container or a
    /// checksum calculation when deparsed. See
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    ///
    /// @return true when @p f is a field slice covered by @p clot and at least part of @p f
    ///         will be overwritten by a PHV container or a checksum calculation when deparsed.
    bool slice_overwritten(const PhvInfo &phvInfo, const Clot *clot,
                           const PHV::FieldSlice *f) const;

    /// Determines whether a field slice in a CLOT will be overwritten by a PHV container when
    /// deparsed. See
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    ///
    /// @return true when @p f is a field slice covered by @p clot and at least part of @p f
    ///         will be overwritten by a PHV container when deparsed.
    bool slice_overwritten_by_phv(const PhvInfo &phvInfo, const Clot *clot,
                                  const PHV::FieldSlice *f) const;

 private:
    /// Determines which bits in the given field slice @p f will be overwritten by a PHV
    /// container or a checksum calculation when deparsed. See
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    ///
    /// @return a bit vector with the given @p Order, indicating those bits in @p f that
    ///         will be overwritten.
    template <Endian Order = Endian::Network>
    bitvec bits_overwritten(const PhvInfo &phvInfo, const Clot *clot,
                            const PHV::FieldSlice *f) const;

    /// Determines which bits in the given field slice @p f will be overwritten by a PHV
    /// container when deparsed. See
    /// \ref clot_alloc_and_metric "CLOT allocator and metric" (README.md).
    ///
    /// @return a bit vector with the given @p Order, indicating those bits in @p f that
    ///         will be overwritten.
    template <Endian Order = Endian::Network>
    bitvec bits_overwritten_by_phv(const PhvInfo &phvInfo, const Clot *clot,
                                   const PHV::FieldSlice *f) const;

 public:
    /// Determines whether @p h is a header that might be added by MAU.
    bool is_added_by_mau(cstring h) const;

    /// @return the CLOT-allocated slices that overlap with the given @p slice, mapped to the
    /// corresponding CLOTs.
    assoc::map<const PHV::FieldSlice *, Clot *, PHV::FieldSlice::Greater> *slice_clots(
        const PHV::FieldSlice *slice) const;

    /// @return the CLOT-allocated slices of the given @p field, mapped to the CLOTs containing
    /// those slices.
    assoc::map<const PHV::FieldSlice *, Clot *, PHV::FieldSlice::Greater> *slice_clots(
        const PHV::Field *field) const {
        return slice_clots(new PHV::FieldSlice(field));
    }

    /// @return the CLOT containing the entirety of the given field, or nullptr if no such CLOT
    /// exists.
    Clot *whole_field_clot(const PHV::Field *field) const;

    /// @return true if the given @p slice is covered by the given @p clot.
    bool clot_covers_slice(const Clot *clot, const PHV::FieldSlice *slice) const;

    std::string print(const PhvInfo *phvInfo = nullptr) const;

 private:
    void add_alias(const PHV::Field *f1, const PHV::Field *f2);

    void clear();

    /// Finds the paths with the largest number of CLOTs allocated.
    ///
    /// @param memo is a memoization table that maps each visited parser state to the corresponding
    ///     result of the recursive call.
    ///
    /// @return the maximum number of CLOTs allocated on paths starting at the given @p state,
    ///     paired with the aggregate set of nodes on those maximal paths.
    //
    // DANGER: This function assumes the parser graph is a DAG.
    std::pair<unsigned, ordered_set<const IR::BFN::ParserState *> *> *find_largest_paths(
        const std::map<cstring, std::set<const Clot *>> &parser_state_to_clots,
        const IR::BFN::ParserGraph *graph, const IR::BFN::ParserState *state,
        std::map<const IR::BFN::ParserState *,
                 std::pair<unsigned, ordered_set<const IR::BFN::ParserState *> *> *> *memo =
            nullptr) const;
};

/**
 * \ingroup parde
 */
class CollectClotInfo : public Inspector {
    const PhvInfo &phv;
    ClotInfo &clotInfo;
    const PragmaDoNotUseClot &pragmaDoNotUseClot;
    Logging::FileLog *log = nullptr;

 public:
    explicit CollectClotInfo(const PhvInfo &phv, ClotInfo &clotInfo,
                             const PragmaDoNotUseClot &pragmaDoNotUseClot)
        : phv(phv), clotInfo(clotInfo), pragmaDoNotUseClot(pragmaDoNotUseClot) {}

 private:
    Visitor::profile_t init_apply(const IR::Node *root) override;

    /// Collects the set of fields extracted.
    bool preorder(const IR::BFN::Extract *extract) override;
    bool preorder(const IR::BFN::ParserZeroInit *zero_init) override;

    /// Collects the set of header stacks extracted in loop states
    bool preorder(const IR::HeaderStackItemRef *hs) override;

    /// Collects the set of POV bits on which each field's emit is predicated.
    bool preorder(const IR::BFN::EmitField *emit) override;

    /// Identifies checksum fields, collects the set of fields over which checksums are computed,
    /// and collects the set of POV bits on which each field's emit is predicated.
    bool preorder(const IR::BFN::EmitChecksum *emit) override;

    /// Uses @a fields_to_pov_bits to identify pseudoheaders.
    void postorder(const IR::BFN::Deparser *deparser) override;

    /// Helper.
    ///
    /// Does nothing if @p fields is empty. Otherwise, adds a new pseudoheader with the given
    /// @p pov_bits and @p fields, if one hasn't already been allocated.
    void add_pseudoheader(
        const PovBitSet pov_bits, const std::vector<const PHV::Field *> fields,
        std::map<std::pair<const PovBitSet, const std::vector<const PHV::Field *>>,
                 const Pseudoheader *> &allocated);

    /// Collects aliasing information.
    bool preorder(const IR::BFN::AliasMember *alias) override {
        add_alias_field(alias);
        return true;
    }

    /// Collects aliasing information.
    bool preorder(const IR::BFN::AliasSlice *alias) override {
        add_alias_field(alias);
        return true;
    }

    void add_alias_field(const IR::Expression *alias);

    /// Collects the set of headers that are made valid by MAU.
    bool preorder(const IR::MAU::Instruction *instruction) override;

    void end_apply(const IR::Node *root) override;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CLOT_INFO_H_ */
