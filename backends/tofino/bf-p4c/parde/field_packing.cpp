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
