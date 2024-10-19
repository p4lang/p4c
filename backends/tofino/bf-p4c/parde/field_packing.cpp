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

#include "bf-p4c/parde/field_packing.h"

#include "bf-p4c/parde/marshal.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace BFN {

using namespace P4;

void FieldPacking::appendField(const IR::Expression *field, unsigned width, gress_t gress) {
    appendField(field, cstring(), width, gress);
}

void FieldPacking::appendField(const IR::Expression *field, cstring source, unsigned width,
                               gress_t gress) {
    BUG_CHECK(field != nullptr, "Packing null field?");
    fields.push_back(PackedItem(field, gress, source, width));
    totalWidth += width;
}

void FieldPacking::appendPadding(unsigned width) {
    if (width == 0) return;
    if (!fields.empty() && fields.back().isPadding()) {
        fields.back().width += width;
    } else {
        fields.push_back(PackedItem::makePadding(width));
    }
    totalWidth += width;
}

void FieldPacking::append(const FieldPacking &packing) {
    for (auto item : packing.fields) {
        if (item.isPadding())
            appendPadding(item.width);
        else
            appendField(item.field, item.source, item.width, item.gress);
    }
}

void FieldPacking::padToAlignment(unsigned alignment, unsigned phase /* = 0 */) {
    BUG_CHECK(alignment != 0, "Zero alignment is invalid");
    const unsigned currentPhase = totalWidth % alignment;
    unsigned desiredPhase = phase % alignment;
    if (currentPhase > desiredPhase)
        desiredPhase += alignment;  // Need to advance enough to "wrap around".
    appendPadding(desiredPhase - currentPhase);
}

void FieldPacking::clear() {
    fields.clear();
    totalWidth = 0;
}

bool FieldPacking::containsFields() const {
    for (auto item : fields)
        if (!item.isPadding()) return true;
    return false;
}

bool FieldPacking::isAlignedTo(unsigned alignment, unsigned phase /* = 0 */) const {
    BUG_CHECK(alignment != 0, "Zero alignment is invalid");
    return totalWidth % alignment == phase % alignment;
}

IR::BFN::ParserState *FieldPacking::createExtractionState(
    gress_t gress, cstring stateName, const IR::BFN::ParserState *finalState) const {
    BUG_CHECK(totalWidth % 8 == 0,
              "Creating extraction states for non-byte-aligned field packing?");

    IR::Vector<IR::BFN::ParserPrimitive> extracts;
    unsigned currentBit = 0;
    size_t pre_padding = 0;
    for (auto &item : fields) {
        if (!item.isPadding()) {
            auto *extract = new IR::BFN::Extract(
                item.field, new IR::BFN::PacketRVal(StartLen(currentBit, item.width), false));
            extract->marshaled_from =
                MarshaledFrom(item.gress, item.field->toString(), pre_padding);
            extracts.push_back(extract);
            pre_padding = 0;
        } else {
            pre_padding = item.width;
        }
        currentBit += item.width;
    }

    return new IR::BFN::ParserState(
        stateName, gress, extracts, {},
        {new IR::BFN::Transition(match_t(), totalWidth / 8, finalState)});
}

}  // namespace BFN

std::ostream &operator<<(std::ostream &out, const BFN::FieldPacking *packing) {
    if (packing == nullptr) {
        out << "(null field packing)" << std::endl;
        return out;
    }

    out << "Field packing: {" << std::endl;
    for (auto item : packing->fields) {
        if (item.isPadding())
            out << " - (padding) ";
        else
            out << " - " << item.field << " = " << item.source << " ";
        out << "(width: " << item.width << " bits)" << std::endl;
    }
    out << "} (total width: " << packing->totalWidth << " bits)" << std::endl;
    return out;
}
