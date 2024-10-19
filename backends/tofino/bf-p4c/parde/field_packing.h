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

#ifndef BF_P4C_PARDE_FIELD_PACKING_H_
#define BF_P4C_PARDE_FIELD_PACKING_H_

#include <vector>

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/gress.h"
#include "lib/cstring.h"

namespace P4 {
namespace IR {
namespace BFN {
class ParserState;
}  // namespace BFN
class Expression;
}  // namespace IR
}  // namespace P4

namespace BFN {

using namespace P4;

/**
 * A field packing format, consisting of a sequence of spans of bits which are
 * either mapped to a field or are treated as padding.
 */
struct FieldPacking {
    struct PackedItem {
        /// The packed field, or null if this is a padding item.
        const IR::Expression *field;
        gress_t gress;

        /// For phase 0, the logical source of this field - generally an action
        /// parameter. This is exposed to the control plane.
        cstring source;

        /// This item's width in bits.
        unsigned width;

        bool isPadding() const { return field == nullptr; }

        explicit PackedItem(unsigned width) : field(nullptr), width(width) {}
        PackedItem(const IR::Expression *field, gress_t gress, cstring source, unsigned width)
            : field(field), gress(gress), source(source), width(width) {}

        static PackedItem makePadding(unsigned width) { return PackedItem(width); }
    };

    typedef std::vector<PackedItem>::iterator iterator;
    typedef std::vector<PackedItem>::const_iterator const_iterator;

    iterator begin() { return fields.begin(); }
    iterator end() { return fields.end(); }
    const_iterator begin() const { return fields.begin(); }
    const_iterator end() const { return fields.end(); }

    /// Iterate over each range of bits in the packing format which is occupied
    /// by a field. The callback function should have a prototype like:
    ///   `f(range of bits, field, field source)`
    /// The ordering of the range of bits is specified by @tparam Order.
    template <Endian Order, typename Func>
    void forEachField(Func func) const {
        int posBits = 0;
        for (auto &field : fields) {
            if (field.isPadding()) {
                posBits += field.width;
                continue;
            }
            const auto fieldRange =
                nw_bitrange(StartLen(posBits, field.width)).toOrder<Order>(totalWidth);
            posBits += field.width;
            func(fieldRange, field.field, field.source);
        }
    }

    /**
     * Append a field to the sequence of packed items.
     *
     * @param field  An IR object corresponding to the field. Generally this is
     *               an IR::Member of a header or struct, but that's not
     *               required.
     * @param source If non-empty, a string identifying the source of the field,
     *               for use by the control plane. This is primarily intended for
     *               the phase 0 table, since the control plane API we generate
     *               uses the names of the action parameter and not the metadata
     *               fields we assign those parameters to.
     * @param width  The width in bits of the field.
     * @param gress  The gress of the field, if it matters.
     */
    void appendField(const IR::Expression *field, cstring source, unsigned width,
                     gress_t gress = INGRESS);
    void appendField(const IR::Expression *field, unsigned width, gress_t gress = INGRESS);

    /// Appends the specified number of bits of padding to the sequence of
    /// packed items.
    void appendPadding(unsigned width);

    /// Appends another sequence of packed items to this one.
    void append(const FieldPacking &packing);

    /**
     * Appends enough padding to make the total width of this sequence a
     * multiple of the provided alignment (in bits).
     *
     * @param alignment  The desired alignment - e.g., 32 bits.
     * @param phase      If provided, an additional offset to the alignment -
     *                   e.g. a 32 bit alignment with a 2 bit phase would add
     *                   padding until the total width was 2 bits after a
     *                   multiple of 32 bits.
     */
    void padToAlignment(unsigned alignment, unsigned phase = 0);

    /// Removes all packed items from this sequence.
    void clear();

    /// @return true if this sequence contains any fields; if it only contains
    /// padding or is totally empty, returns false.
    bool containsFields() const;

    /// @return true if this sequence's total width is a multiple of the
    /// provided alignment (in bits), possibly with an additional bit offset.
    /// @see padToAlignment.
    bool isAlignedTo(unsigned alignment, unsigned phase = 0) const;

    /**
     * Creates a parser state that extract the fields in this sequence from the
     * input buffer and into the destination specified by each packed item's
     * 'field' member.
     *
     * @param gress  Which thread these states are intended for.
     * @param stateName  The name to use for the generated state.
     * @param finalState  The state the generated parser should branch to after
     *                    its work is complete.
     * @return the root of the resulting parser program.
     */
    IR::BFN::ParserState *createExtractionState(gress_t gress, cstring stateName,
                                                const IR::BFN::ParserState *finalState) const;

    /// The sequence of packed items (fields and padding).
    std::vector<PackedItem> fields;

    /// The total width of all packed items in the sequence.
    unsigned totalWidth = 0;
};

}  // namespace BFN

std::ostream &operator<<(std::ostream &out, const BFN::FieldPacking *packing);

#endif /* BF_P4C_PARDE_FIELD_PACKING_H_ */
