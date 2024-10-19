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

#ifndef BF_P4C_PHV_PHV_FIELDS_H_
#define BF_P4C_PHV_PHV_FIELDS_H_

#include <functional>
#include <limits>
#include <optional>

#include <boost/range/irange.hpp>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/thread_visitor.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/lib/cmp.h"
#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/phv/constraints/constraints.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "ir/ir.h"
#include "lib/algorithm.h"
#include "lib/map.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/range.h"
#include "lib/safe_vector.h"
#include "lib/symbitmatrix.h"

using namespace P4;

struct FieldAlignment {
    explicit FieldAlignment(nw_bitrange bitLayout);
    explicit FieldAlignment(le_bitrange bitLayout);

    bool operator==(const FieldAlignment &other) const;
    bool operator!=(const FieldAlignment &other) const;

    bool isByteAligned() const { return align == 0; }

    /// bit in byte alignment of the field in little endian order
    unsigned align;
};

std::ostream &operator<<(std::ostream &, const FieldAlignment &);

namespace PHV {

using SolitaryReason = Constraints::SolitaryConstraint::SolitaryReason;
using DigestType = Constraints::DigestConstraint::DigestType;
using AlignmentReason = Constraints::AlignmentConstraint::AlignmentReason;

class FieldSlice;

enum class FieldKind : unsigned short {
    header = 0,    // header fields
    metadata = 1,  // metadata fields
    pov = 2        // POV fields, eg. $valid
};

enum class FieldAccessType { NONE = 0, R = 1, W = 2, RW = 3 };

/// Represents a PHV-allocation context: a parser, a table, or a deparser.
class AllocContext {
 public:
    enum class Type { PARSER, TABLE, DEPARSER };
    Type type;

    /// Populated iff type is TABLE.
    const IR::MAU::Table *table;

 private:
    /// Creates a context for a parser or a deparser.
    explicit AllocContext(Type type) : type(type), table(nullptr) {
        BUG_CHECK(type != Type::TABLE, "Improper usage of PHV::AllocContext interface");
    }

    /// Creates a table context.
    explicit AllocContext(const IR::MAU::Table *table) : type(Type::TABLE), table(table) {
        BUG_CHECK(table, "Improper usage of PHV::AllocContext interface");
    }

 public:
    bool is_parser() const { return type == Type::PARSER; }
    bool is_deparser() const { return type == Type::DEPARSER; }
    bool is_table() const { return type == Type::TABLE; }
    cstring typeStr() const {
        if (is_parser())
            return "PARSER"_cs;
        else if (is_deparser())
            return "DEPARSER"_cs;
        else if (is_table())
            return "TABLE"_cs;
        return "NONE"_cs;
    }

    static const AllocContext *PARSER;
    static const AllocContext *DEPARSER;

    /// Factory method for producing an AllocContext from a Unit.
    ///
    /// @param unit is expected to be a parser, a deparser, or a table. Anything else is a compiler
    ///             bug.
    static const AllocContext *of_unit(const IR::BFN::Unit *unit) {
        if (!unit) return nullptr;
        if (unit->is<IR::BFN::ParserState>() || unit->is<IR::BFN::Parser>() ||
            unit->is<IR::BFN::GhostParser>())
            return PARSER;
        if (unit->to<IR::BFN::Deparser>()) return DEPARSER;
        if (auto table = unit->to<IR::MAU::Table>()) return new AllocContext(table);
        BUG("Improper usage of PHV::AllocContext interface. Not a parser, deparser, or table: %1%",
            unit);
    }
};
std::ostream &operator<<(std::ostream &, const AllocContext &);

struct FieldOperation {
    bool is_bitwise_op;
    bool is_salu_inst;
    const IR::MAU::Instruction *inst;
    FieldAccessType rw_type;
    le_bitrange range;  // If range.size() == this->size, then the operation is
                        // over the whole field.
    FieldOperation(bool is_bitwise_op, bool is_salu_inst, const IR::MAU::Instruction *inst,
                   FieldAccessType rw_type, le_bitrange range)
        : is_bitwise_op(is_bitwise_op),
          is_salu_inst(is_salu_inst),
          inst(inst),
          rw_type(rw_type),
          range(range) {}
};

class Field;

class Field : public LiftLess<Field> {
 public:
    /** Field name, following this scheme:
     *   - "gress::header.field"
     *   - "gress::header.field[i]" where "i" is a positive integer
     *   - "gress::header.$valid"
     */
    cstring name;

    /// Unique field ID.
    int id = 0;

    /// Whether the Field is ingress or egress.
    gress_t gress = INGRESS;

    /// @returns true if the Field is a ghost field.
    /// TODO: Right now, ghost fields are marked as ingress fields, so we use string comparison
    /// for this method. Ideally, we should use the gress member directly and not have a separate
    /// ghost field.
    bool isGhostField() const {
        if (BackendOptions().arch == "tna") {
            return name.startsWith("ghost::");
        }
        if (name.startsWith("ghost::")) return true;
        // A field valid in both ingress && ghost e.g. ping pong reg does not
        // have a 'ghost::" prefix and above check is not sufficient to identify
        // it to be valid ghost field.
        if (name.startsWith("ingress::")) return false;
        if (name.startsWith("egress::")) return false;
        // FIXME: For no prefix we assume this to be valid in ghost field. Needs
        // to be fixed by relying on 'gress' field rather than prefix
        return true;
    }

    /// @returns true if the field has been generated by the compiler for padding marshalled fields.
    bool isCompilerGeneratedPaddingField() const {
        return name.startsWith("__phv_dummy_padding__");
    }

    /// Total size of Field in bits.
    int size = 0;

    /// The alignment requirement of this field. If std::nullopt, there is no
    /// particular alignment requirement.
    std::optional<FieldAlignment> alignment;
    /// List of alignment sources for this field (mainly for error printing)
    std::list<std::pair<FieldAlignment, Util::SourceInfo>> alignmentSources;

    /// @see Field::validContainerRange().
    nw_bitrange validContainerRange_i = ZeroToMax();

    /// Offset of lsb from lsb (last) bit of containing header.
    int offset = 0;

    /// True if this Field is metadata.
    bool metadata = false;

    /// True if this Field is intrinsic.
    bool intrinsic_i = false;

    /// True if this field needs to invalid when reaching the deparser and is not written by the
    /// user (on Tofino).
    bool invalidate_from_arch_i = false;

    /// True if this Field is metadata bridged from ingress to egress.
    bool bridged = false;

    /// True if this Field is a padding field.
    bool padding = false;

    /// True if this Field can always be overlayable with other fields. Used for padding fields for
    /// bridged metadata.
    bool overlayable = false;

    /// True if this field is emitted by the deparser onto the wire.
    bool emitted_i = false;

    // Define field allocation status.
    enum alloc_code_t {
        EMPTY = 0,
        REFERENCED = 1 << 0,           // Field is referenced
        HAS_PHV_ALLOCATION = 1 << 1,   // Field is at least partially PHV-allocated
        FULLY_PHV_ALLOCATED = 1 << 2,  // Field is fully PHV-allocated
        HAS_CLOT_ALLOCATION = 1 << 3,  // Field is at least partially CLOT-allocated
    };

    typedef unsigned AllocState;

    /// A mirror field points to its field list (one of eight)
    struct mirror_field_list_t {
        Field *member_field;
        int field_list;
        bool operator==(const mirror_field_list_t &rhs) const {
            return rhs.member_field == this->member_field && rhs.field_list == this->field_list;
        }
        bool operator!=(const mirror_field_list_t &rhs) const { return !(*this == rhs); }
    } mirror_field_list = {nullptr, -1};

    /// True if this Field is a validity bit.
    bool pov = false;

    /// If this field is an alias destination, then maintain a pointer to the
    /// alias source. Alias destinations are the canonical representation for
    /// alias sources.
    ///
    /// For example, if we have
    /// ```
    /// @pa_alias("ingress", "h.f", "h.f1")
    /// @pa_alias("ingress", "h.f", "h.f2")
    /// ```
    ///
    /// then h.f (the first field mentioned in each annotation) is an alias
    /// destination for h.f1 and h.f2, which are sources (the second field in each
    /// annotation). The ReplaceAllAliases pass replaces references to h.f1 with
    /// an h.f reference that has h.f1 as its alias source, and similarly for
    /// references to h.f2.
    const PHV::Field *aliasSource = nullptr;

    /// Associate source info to each field
    std::optional<Util::SourceInfo> srcInfo;

    /// Sets the valid starting bit positions (little Endian) for this field.
    /// For example, setStartBits(PHV::Size::b8, bitvec(0,1)) means that the least
    /// significant bit of this field must start at bit 0 in 8b containers.
    void setStartBits(PHV::Size size, bitvec startPositions);

    /// @return the bit positions (little Endian) at which the least significant
    /// bit of this field may be placed.
    bitvec getStartBits(PHV::Size size) const;

    /// @returns the header to which this field belongs.
    cstring header() const { return name.before(strrchr(name, '.')); }

 private:
    /// When set, use this name rather than PHV::Field::name when generating
    /// assembly.
    std::optional<cstring> externalName_i;

    // constraints on this field
    //
    /// True if this Field is part of a flexible struct.
    bool flexible_i = false;
    bool parsed_i = false;    /// true if parsed field
    bool deparsed_i = false;  /// true if field has the deparsed PHV
                              /// constraint. NB: fields with this
                              /// constraint are not necessarily emitted.
                              /// See @ref emitted_i.
    // Solitary Constraint: This field cannot be packed with any other field (although it may share
    // a container with another field through overlay).
    Constraints::SolitaryConstraint solitary_i;

    // Digest Constraint: This field is used in a digest. This constraint also contains the type of
    // digest this field is used in.
    Constraints::DigestConstraint digest_i;

    // Alignment Constraint: This field must be aligned at given offset within a container.
    Constraints::AlignmentConstraint alignment_i;

    bool deparsed_bottom_bits_i = false;  /// true when learning digest, no shifter
    bool deparsed_top_bits_i = false;     /// true when the field must be in the top
                                          /// 16b of a container.
    bool exact_containers_i = false;      /// place in container exactly (no holes)

    bool deparsed_to_tm_i = false;  /// true if field is read by TM
    size_t numNoPack = 0;           /// Number of fields with which this field
                                    /// cannot be packed

    bool is_checksummed_i = false;           /// true for fields used in checksum.
    bool mocha_i = false;                    /// true if field is a candidate for mocha
                                             /// PHV.
    bool dark_i = false;                     /// true if field is a candidate for dark
                                             /// PHV.
    bool deparser_zero_i = false;            /// true if the field is a candidate for the
                                             /// deparser zero optimization.
    bool write_force_immediate_i = false;    /// true if the field is written by action
                                             /// data in a force_immediate table.
    ordered_set<int> wide_arith_start_bit_;  /// Set of start bits of wide arithmetic
                                             /// operations.  For example, if a program
                                             /// had ipv6.dstAddr[127:64] += 1 and
                                             /// ipv6.dstAddr[63:0] += 1, this set would
                                             /// contain 0 and 64.
    bool ignore_alloc_i = false;

    // true if the field is part of Type_FixedSizeHeader, used to represent resubmit/phase0 data.
    bool fixed_size_i = false;

    bool same_container_group_i = false;  /// true if all the slices relative to this
                                          /// field must be part of the same container
                                          /// group.

    /// Used to compute the alloc score. This is not a hard constraint.
    PHV::Size prefer_container_size_i = PHV::Size::null;

    /// true if the field can either be located entirely on a single container or splitted across
    /// multiple of them as long as all of the slices fit with no holes. This can be seen as a
    /// way to relaxe the exact_containers constraints by also allowing a field to be split under
    /// some specific conditions. This was originally added to handle the funnel-shift operation
    /// which can combine multiple containers in the shifting operation as long as the bits are
    /// continuous between the combined containers.
    bool no_holes_i = false;

    bool upcasted_i = false;  /// true if field is upcasted as a source
                              /// using concat or cast operation.

    /// Maximum size of container bytes this field can occupy. -1 if there is no constraint on this
    /// field.
    /// E.g. if an 8-bit field can only occupy a maximum of 2 bytes, this value will be 2, and that
    /// field will be able to occupy a single 16-bit container, or two 8-bit containers (if the
    /// field can be split), or an 8-bit container.
    int maxContainerBytes_i = -1;

    /// Ranges of this field that can not be split.
    /// E.g. in a<32b> = b<32b> + c[0:31]<48b>, [0:31] will be the no_split range for c.
    /// you can create a fieldslice for c as long as it
    /// does not split c[0:31]. E.g. c[32:47] is allowed, but c[15:31] is not.
    std::vector<le_bitrange> no_split_ranges_i;

    /** Marshaled fields are metadata fields serialized between a Tofino deparser and parser.
     *  For example, mirrored field lists can be serialized from ingress deparser (when the mirrored
     *  header is being created) to the ingress parser (when the mirrored header is being
     *processed).
     *
     *  Marshaled fields differ from deparsed fields (i.e. the `deparsed_i` constraint) in that they
     *  are not emitted on the wire.
     *
     *  TODO: Currently, only mirrored field lists are marked as marshaled, but the same
     *  mechanism can be used for learning, recirculation, and bridged metadata.
     **/
    bool is_marshaled_i = false;

    /// If std::nullopt, then the field does not have container id limitation. Otherwise, if not
    /// std::nullopt, this field can only allocated to container that their ids are true in bitvec.
    std::optional<bitvec> limited_container_ids_i = std::nullopt;

    // Maximum field size in the same SuperCluster with no_split constraint.
    // Used by bridged metadata packing to insert padding after the 'no_split'
    // bridged field, if the bridged field is smaller than the related
    // 'no_split' field.
    int no_split_container_size_i = -1;

    /// MAU operations performed on this field.
    safe_vector<FieldOperation> operations_i;

    // Do not allocate this field in phv or clots. Use for local parser variables.
    bool avoid_alloc_i = false;

    /// Maps slices of this field to PHV containers.  Sorted by field MSB
    /// first.
    safe_vector<PHV::AllocSlice> alloc_slice_i;

    friend std::ostream &operator<<(std::ostream &out, const Field &field);

 public:
    bool is_flexible() const { return flexible_i; }
    void set_flexible(bool b) { flexible_i = b; }
    /// @returns true if this field can be placed in TPHV containers.
    bool is_tphv_candidate(const PhvUse &uses) const;
    bool is_mocha_candidate() const { return mocha_i; }
    bool is_dark_candidate() const { return dark_i; }
    bool is_deparser_zero_candidate() const { return deparser_zero_i; }
    void set_mocha_candidate(bool c) { mocha_i = c; }
    void set_dark_candidate(bool c) { dark_i = c; }
    void set_deparser_zero_candidate(bool c) { deparser_zero_i = c; }

    bool is_ignore_alloc() const { return ignore_alloc_i; }
    void set_ignore_alloc(bool b) { ignore_alloc_i = b; }

    bool is_avoid_alloc() const { return avoid_alloc_i; }
    void set_avoid_alloc(bool a) { avoid_alloc_i = a; }

    bool is_padding() const { return padding; }
    void set_padding(bool p) { padding = p; }
    bool is_overlayable() const { return overlayable; }
    void set_overlayable(bool p) { overlayable = p; }
    bool is_fixed_size_header() const { return fixed_size_i; }
    void set_fixed_size_header(bool f) { fixed_size_i = f; }

    bool emitted() const { return emitted_i; }
    void set_emitted(bool b) { emitted_i = b; }

    bool is_upcasted() const { return upcasted_i; }
    void set_upcasted(bool c) { upcasted_i = c; }

    //
    // constraints
    //
    bool parsed() const { return parsed_i; }
    void set_parsed(bool b) { parsed_i = b; }
    /// NB: Fields satisfying the deparsed constraint are not necessarily emitted. To determine
    /// whether a field is emitted by the deparser onto the wire, see @a emitted_i.
    bool deparsed() const { return deparsed_i; }
    void set_deparsed(bool b) { deparsed_i = b; }

    bool is_solitary() const { return solitary_i.hasConstraint(); }
    void set_solitary(uint32_t reason) { solitary_i.addConstraint(reason); }

    const Constraints::SolitaryConstraint &getSolitaryConstraint() const { return solitary_i; }

    bool is_digest() const { return digest_i.hasConstraint(); }
    void set_digest(uint32_t source) { digest_i.addConstraint(source); }

    const Constraints::DigestConstraint &getDigestConstraint() const { return digest_i; }

    // only update the alignment constraint used by bridged packing
    void set_alignment(Constraints::AlignmentConstraint &c) { alignment_i = c; }
    void set_alignment(unsigned r, unsigned v) { alignment_i.addConstraint(r, v); }
    void erase_alignment() { alignment_i.eraseConstraint(); }
    const Constraints::AlignmentConstraint &getAlignmentConstraint() const { return alignment_i; }

    bool deparsed_bottom_bits() const { return deparsed_bottom_bits_i; }
    void set_deparsed_bottom_bits(bool b) { deparsed_bottom_bits_i = b; }
    bool deparsed_top_bits() const { return deparsed_top_bits_i; }
    void set_deparsed_top_bits(bool b) { deparsed_top_bits_i = b; }
    bool exact_containers() const { return exact_containers_i; }
    void set_exact_containers(bool b) { exact_containers_i = b; }
    void set_written_in_force_immediate(bool b) { write_force_immediate_i = b; }
    bool written_in_force_immediate_table() const { return write_force_immediate_i; }

    void set_no_split_container_size(int size) { no_split_container_size_i = size; }
    int no_split_container_size() const { return no_split_container_size_i; }
    bool no_split() const;
    void set_no_split(bool b);
    bool no_split_at(int pos) const;
    bool has_no_split_at_pos() const;
    void set_no_split_at(le_bitrange range);  // The indicated slice cannot be split.

    bool same_container_group() const { return same_container_group_i; }
    void set_same_container_group(bool b) { same_container_group_i = b; }

    bool no_holes() const { return no_holes_i; }
    void set_no_holes(bool b) { no_holes_i = b; }

    void set_prefer_container_size(PHV::Size cnt_size) { prefer_container_size_i = cnt_size; }
    PHV::Size prefer_container_size() const { return prefer_container_size_i; }

    bool used_in_wide_arith() const { return wide_arith_start_bit_.size() > 0; }

    bool bit_used_in_wide_arith(int slice_bit) const {
        for (auto bit : wide_arith_start_bit_) {
            if (slice_bit >= bit && slice_bit < (bit + 64)) return true;
        }
        return false;
    }
    bool bit_is_wide_arith_lo(int slice_bit) const {
        for (auto bit : wide_arith_start_bit_) {
            if (slice_bit < (bit + 32)) return true;
        }
        return false;
    }

    // Only called for least signficant bit of wide arith operation.
    // Returns true if could successfully add constraint.
    bool add_wide_arith_start_bit(int start_bit) {
        for (auto bit : wide_arith_start_bit_) {
            if (bit != start_bit) {
                // conflicting constraint if start bit has to go to both
                // odd and even container locations
                if (bit < start_bit && start_bit < (bit + 64)) return false;
                if (start_bit < bit && bit < (start_bit + 64)) return false;
            }
        }
        wide_arith_start_bit_.insert(start_bit);
        return true;
    }
    std::vector<le_bitrange> no_split_ranges() const { return no_split_ranges_i; }

    bool deparsed_to_tm() const { return deparsed_to_tm_i; }
    void set_deparsed_to_tm(bool b) { deparsed_to_tm_i = b; }

    bool is_marshaled() const { return is_marshaled_i; }
    void set_is_marshaled(bool b) { is_marshaled_i = b; }

    bool is_checksummed() const { return is_checksummed_i; }
    void set_is_checksummed(bool b) { is_checksummed_i = b; }
    bool is_intrinsic() const { return intrinsic_i; }
    void set_intrinsic(bool b) { intrinsic_i = b; }
    bool is_invalidate_from_arch() const { return invalidate_from_arch_i; }
    void set_invalidate_from_arch(bool b) { invalidate_from_arch_i = b; }

    void set_limited_container_ids(const std::optional<bitvec> &ids) {
        limited_container_ids_i = ids;
    }
    const std::optional<bitvec> &limited_container_ids() const { return limited_container_ids_i; }

    // @returns the set of MAU operations on this field.
    const safe_vector<FieldOperation> &operations() const { return operations_i; }

    // @returns the set of MAU operations on this field.
    safe_vector<FieldOperation> &operations() { return operations_i; }

    void set_num_pack_conflicts(size_t no) { numNoPack = no; }

    size_t num_pack_conflicts() const { return numNoPack; }

    bool hasMaxContainerBytesConstraint() const { return maxContainerBytes_i != -1; }

    int getMaxContainerBytes() const { return maxContainerBytes_i; }

    void setMaxContainerBytes(int size) { maxContainerBytes_i = size; }

    /// @returns true if this field is a packet field.
    bool isPacketField() const { return (!metadata && !pov); }

    /// @returns the PHV::AllocSlice in which field @p bit is allocated.  Fails
    /// catastrophically if @p bit is not allocated or not within the range of this
    /// field's size.
    /// FIXME -- should take an AllocContext and FieldUse like foreach_byte below?
    const PHV::AllocSlice &for_bit(int bit) const;

    /** For each byte-aligned container byte of each allocated slice of this
     * field, construct an alloc_slice representing that allocated byte (or
     * fraction thereof) and apply @p fn to it, BUT ONLY if the container is NOT
     * a TPHV container.
     *
     *  For example, suppose a 16b field (f) is allocated as follows:
     *
     *  C8 [4:0]     <— f [15:11]
     *  C8 [7:7]     <— f [10:10]
     *  C16 [9:0]    <— f [9:0]
     *
     *  Where C8 is an 8b container and C16 is a 16b container.
     *
     *  Invoking `f->foreach_byte(1, 14, fn)` should invoke fn on the following alloc_slices (in
     * this order):
     *
     *  C16 [7:1]    <— f [7:1]
     *  C16 [9:8]    <— f [9:8]
     *  C8 [7:7]    <— f [10:10]
     *  C8 [3:0]    <— f [14:11]
     */
    void foreach_byte(le_bitrange r, const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const;
    void foreach_byte(le_bitrange r, const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_byte(r, PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    void foreach_byte(const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_byte(StartLen(0, this->size), ctxt, use, fn, useTblRefs);
    }
    void foreach_byte(const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_byte(PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    /** Equivalent to `foreach_byte(*r, fn)`, or `foreach_byte(StartLen(0,
     * this->size), fn)` when @p r is null.
     *
     * @see foreach_byte(le_bitrange, std::function<void(const PHV::AllocSlice&)>).
     */

    void foreach_byte(const le_bitrange *r, const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_byte(r ? *r : StartLen(0, this->size), ctxt, use, fn, useTblRefs);
    }
    void foreach_byte(const le_bitrange *r, const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                      std::function<void(const PHV::AllocSlice &)> fn,
                      SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_byte(r, PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    /// @returns a vector of PHV::AllocSlice, such that multiple PHV::AllocSlice within the same
    /// byte of the same container are combined into the same new PHV::AllocSlice.
    /// This is necessary because input crossbar allocation combines multiple slices
    /// of the same field in the same container into a single Use object.
    const std::vector<PHV::AllocSlice> get_combined_alloc_bytes(
        const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
        SliceMatch useTblRefs = SliceMatch::DFLT) const;

    /// @returns a vector of PHV::AllocSlice, such that multiple PHV::AllocSlice within the same
    /// container are combined into the same new PHV::AllocSlice, if the ranges of the two
    /// PHV::AllocSlice are contiguous. This is necessary because parser validation checks has this
    /// invariant.
    const std::vector<PHV::AllocSlice> get_combined_alloc_slices(le_bitrange bits,
                                                                 const PHV::AllocContext *ctxt,
                                                                 const PHV::FieldUse *use) const;

    /// Apply @p fn to each PHV::AllocSlice within the specified @p ctxt to which this has been
    /// allocated (if any). @p ctxt can be one of ParserState, Table, Deparser, or
    /// null for no filter. @p use can be READ or WRITE. For now, the context is the
    /// entire pipeline, as PHV allocation is global.
    void foreach_alloc(le_bitrange r, const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
                       std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const;

    void foreach_alloc(le_bitrange r, const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                       std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(r, PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    void foreach_alloc(le_bitrange r, std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(r, (PHV::AllocContext *)nullptr, nullptr, fn, useTblRefs);
    }

    /** Equivalent to `foreach_alloc(StartLen(0, this->size), ctxt, fn)`.
     *
     * @see foreach_alloc(le_bitrange, const IR::BFN::Unit *, const PHV::FieldUse&,
     *                    std::function<void(const PHV::AllocSlice&)>).
     */
    void foreach_alloc(const PHV::AllocContext *ctxt, const PHV::FieldUse *use,
                       std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(StartLen(0, this->size), ctxt, use, fn, useTblRefs);
    }

    void foreach_alloc(const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                       std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    void foreach_alloc(std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc((PHV::AllocContext *)nullptr, nullptr, fn, useTblRefs);
    }

    /** Equivalent to `foreach_alloc(StartLen(0, this->size), ctxt, use, fn)`, or to
     * `foreach_alloc(ctxt, fn)` when @p r is null.
     *
     * @see foreach_alloc(le_bitrange, const IR::BFN::Unit *,
     *                    std::function<void(const PHV::AllocSlice&)>).
     */
    void foreach_alloc(const le_bitrange *r, const PHV::AllocContext *ctxt,
                       const PHV::FieldUse *use, std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(r ? *r : StartLen(0, this->size), ctxt, use, fn, useTblRefs);
    }

    void foreach_alloc(const le_bitrange *r, const IR::MAU::Table *ctxt, const PHV::FieldUse *use,
                       std::function<void(const PHV::AllocSlice &)> fn,
                       SliceMatch useTblRefs = SliceMatch::DFLT) const {
        foreach_alloc(r, PHV::AllocContext::of_unit(ctxt), use, fn, useTblRefs);
    }

    /// @returns the number of distinct container bytes that contain slices of
    /// the @p bits of this field.
    int container_bytes(std::optional<le_bitrange> bits = std::nullopt) const;

    /// Clear any PHV allocation for this field.
    void clear_alloc() { alloc_slice_i.clear(); }

    /// Allocate a slice of this field.
    void add_alloc(const PHV::AllocSlice &alloc) { alloc_slice_i.push_back(alloc); }

    PHV::AllocSlice *add_and_return_alloc(const Field *f, PHV::Container c, int fb, int cb, int w,
                                          const ActionSet &a) {
        alloc_slice_i.emplace_back(f, c, fb, cb, w, a);
        unsigned size = alloc_slice_i.size();
        return &(alloc_slice_i[size - 1]);
    }

    /// Set all allocated slices of this field.
    void set_alloc(const safe_vector<PHV::AllocSlice> &alloc) { alloc_slice_i = alloc; }

    /// @returns the PHV allocation for this field, if any.
    const safe_vector<PHV::AllocSlice> &get_alloc() const { return alloc_slice_i; }

    safe_vector<PHV::AllocSlice> &get_alloc() { return alloc_slice_i; }

    /// @returns the number of allocated slices of this field.
    size_t alloc_size() const { return alloc_slice_i.size(); }

    /// @returns true if there are no allocated slices of this field.
    bool is_unallocated() const { return alloc_slice_i.empty(); }

    /// Sort by field MSB.
    void sort_alloc() {
        std::sort(alloc_slice_i.begin(), alloc_slice_i.end(),
                  [](PHV::AllocSlice l, PHV::AllocSlice r) {
                      if (l.field_slice().lo != r.field_slice().lo)
                          return l.field_slice().lo > r.field_slice().lo;
                      return l.getEarliestLiveness().first > r.getEarliestLiveness().first;
                  });
    }

    /// Update the alignment requirement for this field. Reports an error if
    /// conflicting requirements render the alignment unsatisfiable.
    void updateAlignment(PHV::AlignmentReason, const FieldAlignment &newAlignment,
                         const Util::SourceInfo &newAlignmentSource);

    /// Erase the alignment requirement for this field.
    void eraseAlignment();

    /**
     * Update the valid range of container positions for this field.
     * Reports an error if conflicting requirements render the constraint
     * unsatisfiable.
     *
     * @param newValidRange  A new valid range constraint. This is
     *                       intersected with any existing valid range
     *                       constraint to produce a new overall valid
     *                       container range for this field.
     */
    void updateValidContainerRange(nw_bitrange newValidRange);

    /// Get the external name of this field.  If PHV::Field::externalName is
    /// not std::nullopt, use that; otherwise, use PHV::Field::name.
    cstring externalName() const { return externalName_i ? *externalName_i : name; }

    /// @returns true if this field as an external name set independently of
    /// its name.
    bool hasExternalName() const { return externalName_i != std::nullopt; }

    /// Set the external name of this field, which will be used in place of
    /// PHV::Field::name when generating assembly.
    void setExternalName(cstring name) { externalName_i = name; }

    /// Clear the external name, if any has been set.
    void clearExternalName() { externalName_i = std::nullopt; }

    /** The range of possible bit positions at which this field can be placed
     * in a container, in network order.  For example, suppose we have an 8-bit
     * field with `validContainerRange = [0, 11]` and a 16-bit container.
     *
     *      0              15  (network order)
     *    | ---------------- | container
     *      ^          ^
     *      X          Y
     *
     * The entire field must be placed between X and Y (inclusive).
     *
     * Note that field-->container assignment is usually in *little Endian*.
     * From that perspective the picture looks like:
     *
     *      15             0   (little Endian order)
     *    | ---------------- | container
     *      ^          ^
     *      X          Y
     *
     * And so the field must be placed in what are considered the "upper" bits
     * of the container.
     *
     * TODO: This range always starts at 0, which is an invariant that
     * other parts of the compiler rely on.
     */
    nw_bitrange validContainerRange() const { return validContainerRange_i; }

    /// Returns a range of bits in the header that captures the byte aligned offset of this field
    /// within its header.
    le_bitrange byteAlignedRangeInBits() const {
        int start = 8 * (offset / 8);
        int len = (8 * ROUNDUP(offset + size, 8)) - start;
        return StartLen(start, len);
    }

    /// Utility functions to get field allocation status
    bool hasPhvAllocation(AllocState s) const {
        return (s & HAS_PHV_ALLOCATION) || (s & FULLY_PHV_ALLOCATED);
    }
    bool hasClotAllocation(AllocState s) const { return s & HAS_CLOT_ALLOCATION; }

    /// Determines whether @p s has a PHV allocation or a CLOT allocation.
    bool hasAllocation(AllocState s) const { return hasPhvAllocation(s) || hasClotAllocation(s); }

    bool fullyPhvAllocated(AllocState s) const { return s & FULLY_PHV_ALLOCATED; }
    bool partiallyPhvAllocated(AllocState s) const {
        return hasPhvAllocation(s) && !fullyPhvAllocated(s);
    }
    bool isReferenced(AllocState s) const { return (s & REFERENCED) || hasAllocation(s); }

 private:
    /// Marks the valid starting bit positions (little Endian) for this field.
    /// Valid bit positions may vary depending on container size.

    // TODO: This is currently only used for SALU operands.  However,
    // it's general enough to support bit-in-byte alignment requirements
    // (alignment_i), valid container range requirements, and deparsed_to_tm.
    std::map<PHV::Size, bitvec> startBitsByContainerSize_i;

 public:
    Field() {
        for (auto size : Device::phvSpec().containerSizes())
            startBitsByContainerSize_i[size] = bitvec(0, int(size));
    }

    /// Orders by name.
    //
    // The precise ordering here is unimportant, as long as it is stable across different compiler
    // runs.
    bool operator<(const Field &other) const { return name < other.name; }

    /// setStartBitsToLowerBitsOfBottomByte will set the start bit of this field to be limited to
    /// the [0..x]-th (little endian) bit of any-sized containers, where x equals to
    /// (1) for field.size <= 8-bit, x = 8 - size;
    /// (2) for 8 < field.size <= 16, x = 16 - size;
    /// (3) for 16 < field.size, x = 7.
    void setStartBitsToLowerBitsOfBottomByte();
};

/**
 * The base class for PHV::FieldSlice and PHV::Constant.
 * A field list could have both field slice and constant.
 */
class AbstractField {
 public:
    AbstractField() {}
    virtual ~AbstractField() {}
    virtual int size() const = 0;
    virtual const PHV::Field *field() const = 0;
    virtual const le_bitrange &range() const = 0;
    template <typename T>
    const T &as() const {
        return dynamic_cast<const T &>(this);
    }
    template <typename T>
    const T *to() const {
        return dynamic_cast<const T *>(this);
    }
    template <typename T>
    bool is() const {
        return to<T>() != nullptr;
    }
    static AbstractField *create(const PhvInfo &, const IR::Expression *);
};

/**
 * Represent a constant value in a field list.
 */
class Constant : public AbstractField {
    le_bitrange range_i;

 public:
    explicit Constant(const IR::Constant *value) : value(value) {
        range_i = {0, value->type->width_bits() - 1};
    }

    int size() const override { return value->type->width_bits(); }
    const PHV::Field *field() const override { return nullptr; }
    const le_bitrange &range() const override { return range_i; }
    const IR::Constant *value;
};

/** Represents a slice (range of bits) of a PHV::Field.  Constraints on the
 * field that are related to position, like alignment, are tailored for
 * each slice.
 */
class FieldSlice : public AbstractField, public LiftCompare<FieldSlice> {
    // There is no reason for a FieldSlice to change the field it is representing, so make this
    // const (also used in ActionPhvConstraints)
    const PHV::Field *field_i;
    le_bitrange range_i;
    std::optional<FieldAlignment> alignment_i = std::nullopt;
    nw_bitrange validContainerRange_i = ZeroToMax();

 public:
    FieldSlice() : field_i(nullptr), range_i(StartLen(0, 0)) {}

    FieldSlice(const Field *field, le_bitrange range);

    /// Create a slice that holds the entirety of @p field.
    explicit FieldSlice(const Field *field)
        : FieldSlice(field, le_bitrange(StartLen(0, field->size))) {}

    /// Creates a subslice of the field of @p slice from @p range.lo to @p range.hi.
    FieldSlice(FieldSlice slice, le_bitrange range) : FieldSlice(slice.field(), range) {
        BUG_CHECK(slice.range().contains(range),
                  "Trying to create field sub-slice larger than the original slice");
    }

    explicit operator bool() const { return field_i != nullptr; }

    /// Returns a range of bits in the header that captures the byte aligned offset of this slice
    /// within its header.
    /// E.g. Suppose a field f is at offset 10 within its header type and its size is 12 bits. The
    /// byte aligned range for f[0:7] will be [8, 24].
    le_bitrange byteAlignedRangeInBits() const {
        int start = 8 * ((field_i->offset + range_i.lo) / 8);
        int len = (8 * ROUNDUP(field_i->offset + range_i.hi + 1, 8)) - start;
        return StartLen(start, len);
    }

    cstring shortString() const {
        if (is_whole_field()) return field_i->name;

        std::stringstream ss;
        ss << field_i->name << "[" << range_i.hi << ":" << range_i.lo << "]";
        return ss.str();
    }

    bool operator==(const FieldSlice &other) const override {
        return field_i == other.field() && range_i == other.range();
    }

    bool operator<(const FieldSlice &other) const override {
        if (field_i != other.field()) return Field::less(field_i, other.field());
        if (range_i.lo != other.range().lo) return range_i.lo < other.range().lo;
        return range_i.hi < other.range().hi;
    }

    /// Whether the Field is ingress or egress.
    gress_t gress() const { return field_i->gress; }

    /// Total size of FieldSlice in bits.
    int size() const override { return range_i.size(); }

    /// The alignment requirement of this field slice. If std::nullopt, there is
    /// no particular alignment requirement.
    std::optional<FieldAlignment> alignment() const { return alignment_i; }

    /// See documentation for `Field::validContainerRange()`.
    nw_bitrange validContainerRange() const { return validContainerRange_i; }

    /// Kind of field of this slice.
    FieldKind kind() const {
        // TODO: PHV::Field::metadata and PHV::Field::pov should be
        // replaced by FieldKind.
        if (field_i->pov)
            return FieldKind::pov;
        else if (field_i->metadata)
            return FieldKind::metadata;
        else
            return FieldKind::header;
    }

    /// @returns the field this is a slice of.
    const PHV::Field *field() const override { return field_i; }

    /// @returns the bits of the field included in this field slice.
    const le_bitrange &range() const override { return range_i; }

    /// @returns true if this field slice contains the entirety of its field.
    bool is_whole_field() const { return range_i == StartLen(0, field_i->size); }

    /// @returns the bit positions (little Endian) at which the least significant
    /// bit of this field may be placed.
    bitvec getStartBits(PHV::Size size) const;

    /// @returns true if the slice can be allocated to a TPHV container.
    bool is_tphv_candidate(const PhvUse &uses) const;

    // methods that just forward to the underlying PHV::Field method
    int container_bytes() const { return field_i->container_bytes(range_i); }
    template <class FN>
    void foreach_byte(const PHV::AllocContext *ctxt, const PHV::FieldUse *use, FN fn) const {
        field_i->foreach_byte(range_i, ctxt, use, fn);
    }
    template <class FN>
    void foreach_byte(const IR::MAU::Table *ctxt, const PHV::FieldUse *use, FN fn) const {
        field_i->foreach_byte(range_i, PHV::AllocContext::of_unit(ctxt), use, fn);
    }

    bool is_unallocated() const { return field_i->is_unallocated(); }

    friend size_t hash_value(const FieldSlice &fs) {
        size_t h = 0;
        boost::hash_combine(h, fs.field()->id);
        boost::hash_combine(h, fs.range().lo);
        boost::hash_combine(h, fs.range().hi);
        return h;
    }
};

struct FieldRange {
    cstring field_name;
    le_bitrange range;
    ordered_set<FieldRange> conflicts;

    bool operator==(const FieldRange &other) const {
        return (field_name == other.field_name) && range == other.range;
    }

    bool operator<(const FieldRange &other) const {
        if (field_name != other.field_name) return (field_name < other.field_name);
        if (range.lo != other.range.lo) return range.lo < other.range.lo;
        return range.hi < other.range.hi;
    }

    bool has_conflict(const FieldRange fld_range) const { return (conflicts.count(fld_range) > 0); }
};

/// PackingLayout represents a packing constraint that specifies slices of fields must be
/// allocated in the specified layout, similar to header layout constraints.
struct PackingLayout {
    using FieldRange = std::pair<const PHV::Field *, le_bitrange>;
    /// FieldRangeOrPadding is a helper class to store either
    /// a slice of field or an integer representing the number of padding bits.
    class FieldRangeOrPadding {
        std::optional<FieldRange> slice;
        int padding = 0;

     public:
        explicit FieldRangeOrPadding(const FieldRange &fs) : slice(fs) {}
        explicit FieldRangeOrPadding(int n_padding_bits) : padding(n_padding_bits) {}
        bool is_fs() const { return slice.has_value(); }
        const FieldRange &fs() const { return *slice; }
        int size() const {
            if (is_fs())
                return slice->second.size();
            else
                return padding;
        }
    };

    /// actual layout
    safe_vector<FieldRangeOrPadding> layout;
    /// gress
    gress_t gress;
};

std::ostream &operator<<(std::ostream &out, const Field &);
std::ostream &operator<<(std::ostream &out, const Field *);
std::ostream &operator<<(std::ostream &out, const PackingLayout &);
std::ostream &operator<<(std::ostream &out, const PackingLayout *);

}  // namespace PHV

/// Expresses the constraints for inserting a table into the IR. The first element of the pair
/// is the set of tables that must come before the inserted table; the second element
/// identifies tables that must come after. Here, UniqueIds (as returned by
/// IR::MAU::Table::pp_unique_id) are used to identify tables in the pipeline.
using InsertionConstraints = std::pair<std::set<UniqueId>, std::set<UniqueId>>;

/// Maps tables to their corresponding constraints for insertion into the IR.
using ConstraintMap = ordered_map<const IR::MAU::Table *, InsertionConstraints>;

/**
 * PhvInfo stores information about the PHV-backed storage in the program -
 * fields of header and metadata instances, header stacks, TempVars, and POV
 * bits. These items are all represented as PHV::Field objects.
 *
 * Prior to PHV allocation, PhvInfo provides a central place to discover
 * information about Fields; this includes their name, size and
 * alignment, the ways in which they're used in the program and the PHV
 * allocation constraints that apply to them.
 *
 * After PHV allocation, PhvInfo additionally stores the allocation results for
 * each Field.
 *
 * PhvInfo is read throughout the backend, but it should be written to only by
 * CollectPhvInfo and the PHV analysis and PHV bind passes. If you need to store
 * new information in PhvInfo, add a pass that collects it from the IR in
 * CollectPhvInfo. @see CollectPhvInfo for more information.
 */
class PhvInfo {
 public:
    // min-stage-based mapping from table to stages.
    static ordered_map<cstring, std::set<int>> table_to_min_stages;
    // physical-stage-based mapping from table to physical stages of the last table placement.
    static ordered_map<cstring, std::set<int>> table_to_physical_stages;
    static int deparser_stage;
    static bool darkSpillARA;
    static const bool DARK_SPILL_ARA_DEFAULT = true;

    /// Pretty-print all fields
    struct DumpPhvFields : public Visitor {
        const PhvInfo &phv;
        const PhvUse &uses;

        explicit DumpPhvFields(const PhvInfo &phv, const PhvUse &uses) : phv(phv), uses(uses) {}

        const IR::Node *apply_visitor(const IR::Node *n, const char *) override;
        /** Prints a histogram of all field sizes (number of fields of a particular size) in a
         * particular gress. Also prints number of fields and total number of bits to be allocated
         */
        void generate_field_histogram(gress_t) const;
    };

    /// PHV-related info about structs, i.e. collections of header or metadata fields.
    struct StructInfo {
        /// True if this is a metadata struct; false for headers.
        bool metadata;

        /// Gress of this struct.
        gress_t gress;

        /// PHV::Field ID (i.e. index into `by_id`) of the first field of
        /// this struct.  Struct fields are assigned contiguous IDs.  This field
        /// is not valid when `size == 0`.
        int first_field_id;

        /// Number of fields in this struct. May be `0` for structs with no
        /// fields.
        int size;

        StructInfo(bool metadata, gress_t gress)
            : metadata(metadata), gress(gress), first_field_id(0), size(0) {}
        StructInfo(bool metadata, gress_t gress, int first_field_id, int size)
            : metadata(metadata), gress(gress), first_field_id(first_field_id), size(size) {
            BUG_CHECK(0 <= size, "PhvInfo::StructInfo with negative size");
            BUG_CHECK(size == 0 || 0 <= first_field_id,
                      "PhvInfo::StructInfo with negative first field offset");
        }

        /// Returns the half-open range of field IDs for this struct.
        boost::integer_range<int> field_ids() const {
            return boost::irange(first_field_id, first_field_id + size);
        }
    };

    /// SameContainerAllocConstraint stores a UnionFind for field slices that
    /// must be allocated into one container because of PARDE constraints.
    struct SameContainerAllocConstraint {
        using FieldBit = std::pair<const PHV::Field *, int>;
        UnionFind<FieldBit> same_byte_bits;

        // return true if the two field slice must share a container.
        bool same_container(const PHV::FieldSlice &a, const PHV::FieldSlice &b) const;
        void clear() { same_byte_bits.clear(); }
    };

    const SymBitMatrix &field_mutex() const { return field_mutex_i; }
    const SymBitMatrix &metadata_mutex() const { return metadata_mutex_i; }
    const SymBitMatrix &dark_mutex() const { return dark_mutex_i; }
    const SymBitMatrix &deparser_no_pack_mutex() const { return deparser_no_pack_i; }
    const SymBitMatrix &digest_no_pack_mutex() const { return digest_no_pack_i; }
    const SameContainerAllocConstraint &same_container_alloc_constraint() const {
        return same_container_alloc_i;
    }
    SymBitMatrix &field_mutex() { return field_mutex_i; }
    SymBitMatrix &metadata_mutex() { return metadata_mutex_i; }
    SymBitMatrix &dark_mutex() { return dark_mutex_i; }
    SymBitMatrix &deparser_no_pack_mutex() { return deparser_no_pack_i; }
    SymBitMatrix &digest_no_pack_mutex() { return digest_no_pack_i; }

    SymBitMatrix &getBridgedExtractedTogether() { return bridged_extracted_together_i; }
    const SymBitMatrix &getBridgedExtractedTogether() const { return bridged_extracted_together_i; }
    SameContainerAllocConstraint &same_container_alloc_constraint() {
        return same_container_alloc_i;
    }

    bool are_bridged_extracted_together(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return bridged_extracted_together_i(f1->id, f2->id);
    }

    SymBitMatrix &getMutuallyAligned() { return mutually_aligned_i; }
    const SymBitMatrix &getMutuallyAligned() const { return mutually_aligned_i; }
    bool are_mutually_aligned(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return mutually_aligned_i(f1->id, f2->id);
    }

    void addMutuallyAligned(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        mutually_aligned_i(f1->id, f2->id) = true;
    }

    void addDeparserNoPack(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        deparser_no_pack_i(f1->id, f2->id) = true;
    }

    void addFieldNoPack(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        field_no_pack_i(f1->id, f2->id) = true;
    }

    void removeFieldNoPack(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        field_no_pack_i(f1->id, f2->id) = false;
    }

    void addDigestNoPack(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        digest_no_pack_i(f1->id, f2->id) = true;
    }

    void removeDigestNoPack(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        digest_no_pack_i(f1->id, f2->id) = false;
    }

    void addFieldMutex(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        field_mutex_i(f1->id, f2->id) = true;
    }

    void removeFieldMutex(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        field_mutex_i(f1->id, f2->id) = false;
    }

    void addMetadataMutex(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        metadata_mutex_i(f1->id, f2->id) = true;
    }

    void addDarkMutex(const PHV::Field *f1, const PHV::Field *f2) {
        BUG_CHECK(f1 && f2, "No PHV field");
        dark_mutex_i(f1->id, f2->id) = true;
    }

    bool isFieldMutex(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return field_mutex_i(f1->id, f2->id);
    }

    bool isMetadataMutex(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return metadata_mutex_i(f1->id, f2->id);
    }

    bool isDarkMutex(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return dark_mutex_i(f1->id, f2->id);
    }

    bool isDeparserNoPack(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return deparser_no_pack_i(f1->id, f2->id);
    }

    bool isFieldNoPack(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return field_no_pack_i(f1->id, f2->id);
    }

    bool isDigestNoPack(const PHV::Field *f1, const PHV::Field *f2) const {
        BUG_CHECK(f1 && f2, "No PHV field");
        return digest_no_pack_i(f1->id, f2->id);
    }

    unsigned sizeFieldNoPack() { return field_no_pack_i.size(); }

    /// Clear the state maintained corresponding to constant extractors.
    void clearConstantExtractionState() {
        constantExtractedInSameState.clear();
        sameStateConstantExtraction.clear();
    }

    /// Inserts a new field @p f into the UnionFind structure that keeps track of fields written in
    /// the same parser state using constant extractors.
    void insertConstantExtractField(PHV::Field *f) { sameStateConstantExtraction.insert(f); }

    /// Perform a union of sets for fields @p f and @p g in the \a sameStateConstantExtraction
    /// UnionFind struct.
    void mergeConstantExtracts(PHV::Field *f, PHV::Field *g) {
        sameStateConstantExtraction.makeUnion(f, g);
        constantExtractedInSameState[f->id] = true;
        constantExtractedInSameState[g->id] = true;
    }

    const UnionFind<PHV::Field *> &getSameSetConstantExtraction() const {
        return sameStateConstantExtraction;
    }

    UnionFind<PHV::Field *> &getSameSetConstantExtraction() { return sameStateConstantExtraction; }

    bool hasParserConstantExtract(const PHV::Field *f) const {
        return constantExtractedInSameState[f->id];
    }

    // Return the insertion constraints for the AlwaysRunAction tables
    const ordered_map<gress_t, ConstraintMap> &getARAConstraints() const { return alwaysRunTables; }

    // Add insertion constraints for table @tbl in gress @grs
    bool add_table_constraints(gress_t grs, IR::MAU::Table *tbl, InsertionConstraints cnstrs) {
        bool has_constrs =
            (alwaysRunTables.count(grs) ? (alwaysRunTables[grs].count(tbl) ? true : false) : false);

        if (!has_constrs) {
            (alwaysRunTables[grs])[tbl] = cnstrs;
        } else {
            (alwaysRunTables[grs])[tbl].first.insert(cnstrs.first.begin(), cnstrs.first.end());
            (alwaysRunTables[grs])[tbl].second.insert(cnstrs.second.begin(), cnstrs.second.end());
        }

        return !has_constrs;
    }

    // Clear insertion constraints
    void clearARAconstraints() { alwaysRunTables.clear(); }

    static void clearMinStageInfo() { PhvInfo::table_to_min_stages.clear(); }
    static void clearPhysicalStageInfo() { PhvInfo::table_to_physical_stages.clear(); }

    static void resetDeparserStage() { PhvInfo::deparser_stage = -1; }

    static void setDeparserStage(int stage) { PhvInfo::deparser_stage = stage; }

    static int getDeparserStage() { return PhvInfo::deparser_stage; }

    static std::set<int> minStages(const IR::MAU::Table *tbl);
    static std::set<int> physicalStages(const IR::MAU::Table *tbl);

    static void addMinStageEntry(const IR::MAU::Table *tbl, int stage,
                                 bool remove_prev_stages = false);
    static void setPhysicalStages(const IR::MAU::Table *tbl, const std::set<int> &stage);

    static bool hasMinStageEntry(const IR::MAU::Table *tbl);

    static cstring reportMinStages();

    static void resetDarkSpillARA() { darkSpillARA = DARK_SPILL_ARA_DEFAULT; }

    // When a gateway and a table is merged together, we need to make sure that the slices used in
    // the gateway are alive at the table with which it is merged. @returns true if the liveness
    // check allows the merging of the gateway with the table.
    bool darkLivenessOkay(const IR::MAU::Table *gateway, const IR::MAU::Table *t) const;

    // return true if some bit of two fieldslice will share the same container after allocation.
    bool must_alloc_same_container(const PHV::FieldSlice &a, const PHV::FieldSlice &b) const {
        return same_container_alloc_i.same_container(a, b);
    }

 private:  // class PhvInfo
    //
    std::map<cstring, PHV::Field> all_fields;
    /// Maps Field.id to Field.  Also used to generate fresh Field.id values.
    safe_vector<PHV::Field *> by_id;

    /// Maps names of header or metadata structs to corresponding info objects.
    std::map<cstring, StructInfo> all_structs;

    /// Tracks the subset of `all_structs` that are only headers, not header stacks.
    // TODO: what about header unions?
    ordered_map<cstring, StructInfo> simple_headers;

    /// Mapping from containers to the fields using those containers.
    assoc::hash_map<PHV::Container, ordered_set<const PHV::Field *>> container_to_fields;

    /// the dummy padding field names.
    ordered_set<cstring> dummyPaddingNames;

    /// Mapping of alias source to alias destination. Used for building the correct table dependency
    /// graph. Populated by ReinstateAliasSources.
    ordered_map<const PHV::Field *, const PHV::Field *> aliasMap;

    /// Mapping of external name to Field pointers, for fields that have a different external
    /// name.
    ordered_map<cstring, PHV::Field *> externalNameMap;

    /// Mapping of gress to map of AlwaysRun tables to tableSeq  placement constraints
    ordered_map<gress_t, ConstraintMap> alwaysRunTables;

    bool alloc_done_ = false;
    bool pov_alloc_done = false;
    bool trivial_alloc_ = false;  // MAU group constraints not met

    /// Stores the mutual exclusion relationships between different PHV fields
    SymBitMatrix field_mutex_i;

    /// Stores the potential overlayable relationships due to live ranges between different metadata
    /// fields.
    SymBitMatrix metadata_mutex_i;

    SymBitMatrix dark_mutex_i;

    /// Stores all pairs of fields that cannot be packed in the same container
    /// Derived from POV bits of adjacent fields in deparser
    SymBitMatrix deparser_no_pack_i;

    /// field_no_pack_i[i, j] is set to true if field f1 with id i cannot be packed
    /// with another field f2 with id j
    SymBitMatrix field_no_pack_i;

    /// Stores all pairs of fields that cannot be packed in the same container
    /// digest_field, digest_field -> false
    /// digest_field, non-digest_field -> true
    SymBitMatrix digest_no_pack_i;

    /// bridged_extracted_together(f1->id, f2->id) is true if f1 and f2 are both bridged fields and
    /// they are extracted as part of the same byte.
    SymBitMatrix bridged_extracted_together_i;

    /// mutually_aligned(f1->id, f2->id) is true if f1 and f2 must be allocated to container
    /// with same alignment constraint.
    SymBitMatrix mutually_aligned_i;

    /// Mapping of all the fields to the TempVars created. This is needed by the Alias
    /// transformation to automatically add initializations of validity bits for alias sources.
    ordered_map<const PHV::Field *, const IR::TempVar *> fields_to_tempvars_i;

    /// lookup table for fieldslices that must be allocated to one container
    /// due to PARDE constraints.
    SameContainerAllocConstraint same_container_alloc_i;

    /// Set of containers that must be set to 0 (and their container validity bit set
    /// unconditionally to 1) for the deparsed zero optimization.
    std::set<PHV::Container> zeroContainers[2];  // per gress

    /// metadataDeps[t1] = { t_n }, means that live range shrinking and metadata initialization
    /// induce new dependencies from t1 to each member t_n. Note: This is currently not used but
    /// will be necessary once we model these new induced dependencies using the table dependency
    /// graph (instead of adding checks in table placement algorithm).
    ordered_map<cstring, ordered_set<cstring>> metadataDeps;

    // reverseMetadataDeps[t1] = { t_n }, means that all tables in t_n must be placed before we
    // place table t1.
    ordered_map<cstring, ordered_set<cstring>> reverseMetadataDeps;

    // For each of the table pairs in above 'metadataDeps' map, below map holds
    // the corresponding slice causing the dependency
    typedef std::pair<cstring, cstring> tpair;
    ordered_map<tpair, const PHV::FieldSlice *> metadataDepFields;

    // UnionFind struct in PhvInfo for all fields that are written in the same parser state using
    // constants.
    UnionFind<PHV::Field *> sameStateConstantExtraction;

    // Structure to quickly check membership in sameStateConstantExtraction.
    bitvec constantExtractedInSameState;

    void clear();

    void add_hdr(cstring headerName, const IR::Type_StructLike *type, gress_t gress,
                 bool isMetadata);
    void add_struct(cstring structName, const IR::Type_StructLike *type, gress_t gress, bool meta,
                    bool bridged, int offset);
    void addPadding(const IR::Padding *padding, gress_t gress);

    template <typename Iter>
    class iterator {
        Iter it;

     public:
        iterator(Iter i) : it(i) {}  // NOLINT(runtime/explicit)
        bool operator==(iterator a) { return it == a.it; }
        bool operator!=(iterator a) { return it != a.it; }
        iterator &operator++() {
            ++it;
            return *this;
        }
        iterator &operator--() {
            --it;
            return *this;
        }
        decltype(**it) operator*() { return **it; }
        decltype(*it) operator->() { return *it; }
    };

    friend class ClearPhvInfo;
    friend class CollectPhvFields;
    friend struct AllocatePOVBits;
    friend struct MarkBridgedMetadataFields;

    /** Add a 1-bit "hdr.$valid" field for each simple header.  For each header stack, add:
     *  - A "stack[x].$valid" field for each stack element.
     *  - A "stack.$push" field if the push_front primitive is used.
     *  - A "stack.$pop" field if the pop_front primitive is used.
     *  - A "stack.$stkvalid" field.
     *
     * @pre CollectHeaderStackInfo and CollectPhvFields.
     * @post POV fields for all headers added to PhvInfo.
     */
    void allocatePOV(const BFN::HeaderStackInfo &);

 public:  // class PhvInfo
    PhvInfo() {}
    void addTempVar(const IR::TempVar *tempVar, gress_t gress);
    bool isTempVar(const PHV::Field *f) const { return fields_to_tempvars_i.count(f) > 0; }
    const PHV::Field *field(int idx) const {
        return size_t(idx) < by_id.size() ? by_id.at(idx) : 0;
    }
    const PHV::Field *field(const cstring &) const;
    const PHV::Field *field(const IR::Expression *, le_bitrange *bits = 0) const;
    const PHV::Field *field(const IR::Member *, le_bitrange *bits = 0) const;
    PHV::Field *field(int idx) { return (size_t)idx < by_id.size() ? by_id.at(idx) : 0; }
    PHV::Field *field(const cstring &name) {
        return const_cast<PHV::Field *>(const_cast<const PhvInfo *>(this)->field(name));
    }
    PHV::Field *field(const IR::Expression *e, le_bitrange *bits = 0) {
        return const_cast<PHV::Field *>(const_cast<const PhvInfo *>(this)->field(e, bits));
    }
    PHV::Field *field(const IR::Member *fr, le_bitrange *bits = 0) {
        return const_cast<PHV::Field *>(const_cast<const PhvInfo *>(this)->field(fr, bits));
    }
    bool has_struct_info(cstring name) const;
    void get_hdr_fields(cstring name_, ordered_set<const PHV::Field *> &flds) const;
    cstring full_hdr_name(const cstring &name) const;
    const PhvInfo::StructInfo *hdr(const cstring &name_) const;

    const StructInfo struct_info(cstring name) const;
    const StructInfo struct_info(const IR::HeaderRef *hr) const {
        return struct_info(hr->toString());
    }
    const std::map<cstring, PHV::Field> &get_all_fields() const { return all_fields; }
    size_t num_fields() const { return all_fields.size(); }
    /// @returns the TempVar pointer corresponding to @p f, if the underlying expression
    /// for field @p f is a TempVar. @returns nullptr otherwise.
    const IR::TempVar *getTempVar(const PHV::Field *f) const;

    PHV::Field *add(cstring fieldName, gress_t gress, int size, int offset, bool isMetadata,
                    bool isPOV, bool bridged = false, bool isPad = false,
                    bool isOverlayable = false, bool isFlexible = false,
                    bool isFixedSizeHeader = false,
                    std::optional<Util::SourceInfo> srcInfo = std::nullopt);

    PHV::Field *create_dummy_padding(size_t sz, gress_t gress, bool overlayable = true);

    std::vector<PHV::AllocSlice> get_alloc(const IR::Expression *f,
                                           const PHV::AllocContext *ctxt = nullptr,
                                           const PHV::FieldUse *use = nullptr) const;

    std::vector<PHV::AllocSlice> get_alloc(const PHV::Field *phv_field, le_bitrange *bits = nullptr,
                                           const PHV::AllocContext *ctxt = nullptr,
                                           const PHV::FieldUse *use = nullptr) const;

    iterator<safe_vector<PHV::Field *>::iterator> begin() { return by_id.begin(); }
    iterator<safe_vector<PHV::Field *>::iterator> end() { return by_id.end(); }
    iterator<safe_vector<PHV::Field *>::const_iterator> begin() const { return by_id.begin(); }
    iterator<safe_vector<PHV::Field *>::const_iterator> end() const { return by_id.end(); }

    bool alloc_done() const { return alloc_done_; }
    bool trivial_alloc() const { return trivial_alloc_; }
    void set_done(bool trivial = false) {
        alloc_done_ = true;
        trivial_alloc_ = trivial;
    }

    /// Container_to_fields map related functions
    /// Clear the container_to_fields map
    void clear_container_to_fields() { container_to_fields.clear(); }
    /** Add new field to a container
     * @param f field stored within container @p c
     * @param c container to store field @p f
     */
    void add_container_to_field_entry(const PHV::Container c, const PHV::Field *f);

    /// @returns the set of fields assigned (partially or entirely) to @p c
    const ordered_set<const PHV::Field *> &fields_in_container(const PHV::Container c) const;

    /// @returns the set of alloc slices assigned to a container @p c.
    std::vector<PHV::AllocSlice> get_slices_in_container(const PHV::Container c) const;

    /** @returns the set of alloc slices assigned to a container @p c within a context.
     *  The context is one of ParserState, Table/Stage, Deparser. A null context represents the
     *  entire pipeline.
     */
    std::vector<PHV::AllocSlice> get_slices_in_container(const PHV::Container c,
                                                         const PHV::AllocContext *ctxt,
                                                         const PHV::FieldUse *use) const;

    /** @returns a bitvec showing all potentially allocated bits within a container,
     *  within the @p ctxt context.
     *  The context is one of ParserState, Table/Stage, Deparser. A null context represents the
     *  entire pipeline.
     */
    bitvec bits_allocated(const PHV::Container, const PHV::AllocContext *ctxt = nullptr,
                          const PHV::FieldUse *use = nullptr) const;

    bitvec bits_allocated(const PHV::Container container, const IR::MAU::Table *ctxt,
                          const PHV::FieldUse *use = nullptr) const {
        return bits_allocated(container, PHV::AllocContext::of_unit(ctxt), use);
    }

    /// @returns a bitvec showing all potentially allocated bits within a container for the given
    /// @p field field, within the @p ctxt context. The context is one of ParserState, Table/Stage,
    /// Deparser. A null context represents the entire pipeline.
    bitvec bits_allocated(const PHV::Container, const PHV::Field *field,
                          const PHV::AllocContext *ctxt = nullptr,
                          const PHV::FieldUse *use = nullptr) const;

    bitvec bits_allocated(const PHV::Container container, const PHV::Field *field,
                          const IR::MAU::Table *ctxt, const PHV::FieldUse *use = nullptr) const {
        return bits_allocated(container, field, PHV::AllocContext::of_unit(ctxt), use);
    }

    /** @returns a bitvec showing the currently allocated bits in a container corresponding to
     * fields simultaneously live with the fields passed in the argument set, within the context
     * @p ctxt.
     * The context is one of ParserState, Table/Stage, Deparser. A null context represents the
     * entire pipeline.
     * Note that one common bitvec is used to represent all fields that may be in a container
     */
    bitvec bits_allocated(const PHV::Container, const ordered_set<const PHV::Field *> &,
                          const PHV::AllocContext *ctxt = nullptr,
                          const PHV::FieldUse *use = nullptr) const;

    bitvec bits_allocated(const PHV::Container container,
                          const ordered_set<const PHV::Field *> &fields, const IR::MAU::Table *ctxt,
                          const PHV::FieldUse *use = nullptr) const {
        return bits_allocated(container, fields, PHV::AllocContext::of_unit(ctxt), use);
    }

    /** @returns the alias source name, if the given expression is either a IR::BFN::AliasMember
     * type or is a slice with a IR::BFN::AliasMember object as the underlying base expression.
     * @returns std::nullopt otherwise.
     */
    std::optional<cstring> get_alias_name(const IR::Expression *expr) const;

    /// Adds an entry to the aliasMap.
    void addAliasMapEntry(const PHV::Field *f1, const PHV::Field *f2) {
        if (aliasMap.count(f1))
            BUG_CHECK(aliasMap[f1] == f2, "Multiple aliases with the same field found");
        aliasMap[f1] = f2;
    }

    /// Adds an entry to the externalNameMap.
    void addExternalNameMapEntry(PHV::Field *f, cstring externalName) {
        externalNameMap[externalName] = f;
    }

    /// @returns the aliasMap. This is empty unless ReinstateAliasSources has been run.
    const ordered_map<const PHV::Field *, const PHV::Field *> &getAliasMap() const {
        return aliasMap;
    }

    /// @return alias destination corresponding to @p f. Return nullptr if no aliasing relationship
    /// exists or if ReinstateAliasSources has not been run.
    const PHV::Field *getAliasDestination(const PHV::Field *f) const {
        if (aliasMap.count(f)) return aliasMap.at(f);
        return nullptr;
    }

    /// @returns the set of deparsed zero containers.
    const std::set<PHV::Container> &getZeroContainers(gress_t gr) const {
        return zeroContainers[gr];
    }

    /// adds container @c to the set of deparsed zero containers.
    void addZeroContainer(gress_t gr, PHV::Container c) {
        zeroContainers[gr].insert(c);
        BUG_CHECK(zeroContainers[gr].size() <= 1,
                  "Only two zero containers allowed: one for each gress");
    }

    /// notes down a required dependence from table t1 to table t2, induced by metadata
    /// initialization to enable live range shrinking. Updates both the metadataDeps and
    /// reverseMetadataDeps members.
    void addMetadataDependency(const IR::MAU::Table *t1, const IR::MAU::Table *t2,
                               const PHV::FieldSlice *slice = nullptr) {
        metadataDeps[t1->name].insert(t2->name);
        reverseMetadataDeps[t2->name].insert(t1->name);
        if (slice) {
            auto table_pair = std::make_pair(t1->name, t2->name);
            if (metadataDepFields.emplace(table_pair, slice).second) {
                LOG5(" Adding tables " << t1->name << " --> " << t2->name
                                       << " with metadata dependent field " << slice);
            }
        }
    }

    /// @returns the set of tables which must be placed before table @p t.
    const ordered_set<cstring> getReverseMetadataDeps(const IR::MAU::Table *t) const {
        static ordered_set<cstring> emptySet;
        if (!reverseMetadataDeps.count(t->name)) return emptySet;
        return reverseMetadataDeps.at(t->name);
    }

    /// @returns a reference to metadataDeps.
    const ordered_map<cstring, ordered_set<cstring>> &getMetadataDeps() const {
        return metadataDeps;
    }

    /// @returns a reference to reverseMetadataDeps.
    const ordered_map<cstring, ordered_set<cstring>> &getReverseMetadataDeps() const {
        return reverseMetadataDeps;
    }

    /// @returns a reference to metadataDepFields.
    const ordered_map<tpair, const PHV::FieldSlice *> &getMetadataDepFields() const {
        return metadataDepFields;
    }

    /// @returns a map of container to slices filtered based on field & slice filtering functions
    typedef ordered_map<PHV::Container, std::vector<PHV::AllocSlice>> ContainterToSliceMap;
    ContainterToSliceMap getContainerToSlicesMap(
        std::function<bool(const PHV::Field *)> *f = nullptr,
        std::function<bool(const PHV::AllocSlice *)> *s = nullptr) const;
};

/**
 * @brief Create and store a PHV::Field for each header and metadata
 * field, and for TempVars. Allocate POV fields for header and metadata
 * instances.
 *
 * We want the information in PhvInfo to be reconstructible at any time in the
 * backend prior to PHV binding. CollectPhvInfo inspects not only the variables
 * and types used in the program, but also e.g. the parser states (to infer
 * which metadata fields are bridged), the digest field lists (to discover PHV
 * allocation constraints for fields which are used there), and general
 * expression (to discover which fields are written). When adding new
 * information to PhvInfo, it's important to *collect that information in
 * CollectPhvInfo rather than adding it manually*; otherwise, it will be lost
 * when CollectPhvInfo is rerun.
 *
 * @pre HeaderStackInfo has been run, so that `BFN::Pipe::headerStackInfo` is
 * available.
 *
 * @post This object contains a PHV::Field for each header and metadata
 * field, and a POV field for each header and metadata instance.
 */
struct CollectPhvInfo : public PassManager {
    explicit CollectPhvInfo(PhvInfo &phv);
};

/** Marks bridged metadata fields that are extracted in the same byte in the
 * egress parser (because of bridged metadata packing) as being
 * extracted-together. This will naturally aid packing those fields in the same
 * container, as the restriction on not packing multiple extracted slices
 * together in the same container can be relaxed for these fields.
 */
class CollectExtractedTogetherFields : public Inspector {
 private:
    PhvInfo &phv_i;
    bool preorder(const IR::BFN::ParserState *state) override;

 public:
    explicit CollectExtractedTogetherFields(PhvInfo &p) : phv_i(p) {}
};

/// Map field to the parser states in which they are extracted or assigned from checksums.
struct MapFieldToParserStates : public Inspector {
    const PhvInfo &phv_i;

    ordered_map<const PHV::Field *, ordered_set<const IR::BFN::ParserState *>>
        field_to_parser_states;

    ordered_map<const PHV::Field *, ordered_set<const IR::BFN::ParserPrimitive *>> field_to_writes;

    ordered_map<PHV::Container, ordered_set<const IR::BFN::ParserState *>>
        container_to_parser_states;

    ordered_map<PHV::Container, ordered_set<const IR::BFN::ParserPrimitive *>> container_to_writes;

    ordered_map<const IR::BFN::ParserPrimitive *, const IR::BFN::ParserState *> write_to_state;

    std::map<const IR::BFN::ParserState *, const IR::BFN::Parser *> state_to_parser;

    ordered_map<const IR::BFN::ParserState *, ordered_set<const IR::BFN::ParserPrimitive *>>
        state_to_writes;

    explicit MapFieldToParserStates(const PhvInfo &phv) : phv_i(phv) {}

    profile_t init_apply(const IR::Node *root) override {
        field_to_parser_states.clear();
        field_to_writes.clear();
        container_to_parser_states.clear();
        container_to_writes.clear();
        write_to_state.clear();
        state_to_parser.clear();
        state_to_writes.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::ParserChecksumWritePrimitive *checksum) override {
        if (auto dest = checksum->getWriteDest()) add_field(dest->field, checksum);
        return true;
    }

    bool preorder(const IR::BFN::Extract *extract) override {
        if (auto lval = extract->dest->to<IR::BFN::FieldLVal>()) add_field(lval->field, extract);
        return true;
    }

 private:
    void add_field(const IR::Expression *field, const IR::BFN::ParserPrimitive *prim) {
        if (auto *f = phv_i.field(field)) {
            auto state = findContext<IR::BFN::ParserState>();
            auto parser = findContext<IR::BFN::Parser>();

            field_to_parser_states[f].insert(state);
            state_to_parser[state] = parser;

            field_to_writes[f].insert(prim);
            write_to_state[prim] = state;
            state_to_writes[state].insert(prim);

            if (phv_i.alloc_done()) {
                PHV::FieldUse use(PHV::FieldUse::WRITE);
                for (auto &alloc : phv_i.get_alloc(field, PHV::AllocContext::PARSER, &use)) {
                    auto cont = alloc.container();
                    container_to_parser_states[cont].insert(state);
                    container_to_writes[cont].insert(prim);
                }
            }
        }
    }
};

void dump(const PhvInfo *);
void dump(const PHV::Field *);

std::ostream &operator<<(std::ostream &, const ordered_set<PHV::Field *> &);
std::ostream &operator<<(std::ostream &, const ordered_set<const PHV::Field *> &);
std::ostream &operator<<(std::ostream &, const PhvInfo &);
std::ostream &operator<<(std::ostream &, const PHV::FieldAccessType &);
std::ostream &operator<<(std::ostream &, const PhvInfo::SameContainerAllocConstraint &);
std::ostream &operator<<(std::ostream &, const PhvInfo::SameContainerAllocConstraint::FieldBit &);

namespace PHV {

std::ostream &operator<<(std::ostream &, const PHV::FieldSlice &sl);
std::ostream &operator<<(std::ostream &, const PHV::FieldSlice *sl);

}  // namespace PHV

// These overloads must be declared directly in `namespace std` to work around
// ADL limitations for lookup of names in sibling namespaces.
namespace std {
ostream &operator<<(ostream &, const list<::PHV::Field *> &);
ostream &operator<<(ostream &, const set<const ::PHV::Field *> &);
}  // namespace std
//
#endif /* BF_P4C_PHV_PHV_FIELDS_H_ */
