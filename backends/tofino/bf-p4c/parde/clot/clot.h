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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_CLOT_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_CLOT_H_

#include <vector>
#include <map>
#include <set>

#include "bf-p4c/ir/gress.h"
#include "bf-p4c/lib/cmp.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

using namespace P4;

namespace P4 {
namespace IR {
namespace BFN {
class ParserState;
class FieldLVal;
}
}
}

namespace PHV {
class Field;
class FieldSlice;
}


namespace P4 {
class cstring;
class JSONGenerator;
class JSONLoader;
}  // namespace P4

class PhvInfo;

using namespace P4;

class Clot final : public LiftCompare<Clot> {
    friend class ClotInfo;
    friend class GreedyClotAllocator;

 public:
    /// JSON deserialization constructor
    Clot() : tag(0), gress(INGRESS) {}

    explicit Clot(gress_t gress) : tag(tag_count[gress]++), gress(gress) {
        BUG_CHECK(gress != GHOST, "Cannot assign CLOTs to ghost gress.");
    }

    explicit Clot(gress_t gress, unsigned tag) : tag(tag), gress(gress) {
        BUG_CHECK(gress != GHOST, "Cannot assign CLOTs to ghost gress.");
    }

    explicit Clot(cstring);

    cstring toString() const;

    /// Equality based on gress and tag.
    bool operator==(const Clot& clot) const {
        return tag == clot.tag && gress == clot.gress;
    }

    /// Lexicographic ordering according to (gress, tag).
    bool operator<(const Clot& clot) const {
        if (gress != clot.gress) return gress < clot.gress;
        return tag < clot.tag;
    }

    /// JSON serialization/deserialization.
    void toJSON(JSONGenerator& json) const;
    static Clot* fromJSON(JSONLoader& json);

    /// Identifies the hardware CLOT associated with this object.
    unsigned tag;

    /// The gress to which this CLOT is assigned.
    gress_t gress;

    const IR::BFN::FieldLVal* pov_bit = nullptr;

    std::optional<unsigned> stack_depth = std::nullopt;

    std::optional<unsigned> stack_inc = std::nullopt;

    unsigned length_in_bytes(cstring parser_state) const;

    /// Returns the bit offset of a field slice within this CLOT. An error occurs if the CLOT does
    /// not contain the exact field slice (i.e., if the given field slice is not completely covered
    /// by this CLOT, or if the CLOT contains a larger slice of the field).
    unsigned bit_offset(cstring parser_state,
                        const PHV::FieldSlice* slice) const;

    /// Returns the byte offset of a field slice within this CLOT. An error occurs if the CLOT does
    /// not contain the exact field slice (i.e., if the given field slice is not completely covered
    /// by this CLOT, or if the CLOT contains a larger slice of the field).
    unsigned byte_offset(cstring parser_state,
                         const PHV::FieldSlice* slice) const;

    /// @return true when @p field is a PHV-allocated field, and this CLOT has (a slice of)
    /// @p field.
    bool is_phv_field(const PHV::Field* field) const;

    /// @return true when @p field is a checksum field, and this CLOT has (a slice of) @p field.
    bool is_checksum_field(const PHV::Field* field) const;

    /// @return true when this CLOT has the exact @p slice.
    bool has_slice(const PHV::FieldSlice* slice) const;

    /// @return true when the first slice in this CLOT is part of the given field @p field.
    bool is_first_field_in_clot(const PHV::Field* field) const;

    /// Indicates the checksum engine (if any) that will deposit in this CLOT
    std::optional<unsigned> csum_unit;

 private:
    void set_slices(cstring parser_state,
                    const std::vector<const PHV::FieldSlice*>& slices);

    enum FieldKind {
        // Designates a modified field.
        MODIFIED,

        // Designates an unmodified field with a PHV allocation.
        READONLY,

        // Designates a field that is replaced with a checksum when deparsed.
        CHECKSUM,

        UNUSED
    };

    /// Adds a field slice to this CLOT.
    void add_slice(cstring parser_state, FieldKind kind,
                   const PHV::FieldSlice* slice);

    /// Field slices in this CLOT, depending on which parser state it begins in.
    /// Field slices present in one parser state may not be present in an other, except
    /// for the first field slice which will always be the same in all parser states.
    std::map<cstring,
             std::vector<const PHV::FieldSlice*>> parser_state_to_slices_;

    std::map<const PHV::FieldSlice*, cstring> slice_to_parser_state_;

    /// All fields that have slices in this CLOT, mapped to their corresponding slice.
    ordered_map<const PHV::Field*, const PHV::FieldSlice*> fields_to_slices_;

    /// All fields that have slices in this CLOT and also have a PHV allocation. This consists of
    /// all modified and read-only fields that have slices in this CLOT.
    std::set<const PHV::Field*> phv_fields_;

    /// Fields that need to be replaced by a checksum when deparsed.
    std::set<const PHV::Field*> checksum_fields_;

 public:
    /// Return map which contains the field slices in the CLOT depending on which parser state
    /// the CLOT began in.
    const std::map<cstring, std::vector<const PHV::FieldSlice*>>& parser_state_to_slices() const {
        return parser_state_to_slices_;
    }

    /// Returns all fields that have slices in this CLOT, mapped to their corresponding slice.
    const ordered_map<const PHV::Field*, const PHV::FieldSlice*>& fields_to_slices() const {
        return fields_to_slices_;
    }

    /// Returns all fields that need to be replaced by a checksum when deparsed.
    const std::set<const PHV::Field*>& checksum_fields() const {
        return checksum_fields_;
    }

    std::map<const PHV::Field*, unsigned> checksum_field_to_checksum_id;

    static std::map<gress_t, int> tag_count;
};

std::ostream& operator<<(std::ostream& out, const Clot& clot);
std::ostream& operator<<(std::ostream& out, const Clot* clot);
P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const Clot& clot);
P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const Clot* clot);

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_CLOT_H_ */
