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

#include "bf-p4c/parde/phase0.h"

#include <algorithm>

#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/parde/field_packing.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/coreLibrary.h"
#include "lib/cstring.h"
#include "lib/indent.h"

std::ostream &operator<<(std::ostream &out, const P4::IR::BFN::Phase0 *p0) {
    if (p0 == nullptr) return out;
    indent_t indent(1);
    out << indent << "phase0_match " << p0->tableName << ":" << std::endl;
    out << ++indent << "p4:" << std::endl;
    out << ++indent << "name: " << p0->tableName << std::endl;
    out << indent << "size: " << p0->size << std::endl;
    out << indent << "preferred_match_type: exact" << std::endl;
    out << indent << "match_type: exact" << std::endl;
    out << --indent << "size: " << p0->size << std::endl;

    // Write out the p4 parameter, which (for phase0) is always
    // For P4-14 = 'ig_intr_md.ingress_port'
    // For P4-16 = '<ingress_intrisic_metadata_t param name>.ingress_port'
    //             must be consistent with bf-rt.json
    out << indent << "p4_param_order:" << std::endl;
    if (p0->keyName.isNullOrEmpty())
        out << ++indent << "ig_intr_md.ingress_port:";
    else
        out << ++indent << p0->keyName << ".ingress_port: ";
    out << "{ type: exact, size: " << Device::portBitWidth() << " }" << std::endl;

    // The phase 0 type defines the format of the phase 0 data (the static
    // per-port metadata, in other words). The driver exposes a table-like
    // API to the control plane, with the phase 0 data for each port exposed
    // as a table entry. The phase 0 type describes to the driver how the
    // fields in these table entries should be formatted so the parser can
    // interpret them correctly. It doesn't need to actually be used in the
    // program, although we do generate code that uses it when translating
    // v1model code to TNA.
    auto *packing = new BFN::FieldPacking;
    for (auto *field : *p0->fields) {
        auto isPadding = bool(field->annotations->getSingle("padding"_cs));

        LOG4("  - " << field->name << " (" << field->type->width_bits() << "b)"
                    << (isPadding ? " (padding)" : ""));

        if (isPadding)
            packing->appendPadding(field->type->width_bits());
        else
            packing->appendField(new IR::StringLiteral(field->name), field->name,
                                 field->type->width_bits());
    }

    auto phase0Size = Device::pardeSpec().bitPhase0Size();
    ERROR_CHECK((packing->totalWidth == phase0Size),
                "Phase 0 type is %1%b, but its size must be exactly "
                "%2%b on %3%",
                packing->totalWidth, phase0Size, Device::name());

    // Write out the field packing format. We have to convert into the LSB-first
    // representation that the assembler uses.
    const nw_bitrange phase0Range = StartLen(0, phase0Size);
    BUG_CHECK(packing->totalWidth == phase0Size,
              "Expected phase 0 field packing to allocate exactly %1% bits", phase0Range.size());
    bool wroteAtLeastOneField = false;
    int posBits = 0;
    out << --indent << "format: {";
    for (auto &field : *packing) {
        BUG_CHECK(field.width > 0, "Empty phase 0 field?");
        const nw_bitrange fieldRange(StartLen(posBits, field.width));
        BUG_CHECK(phase0Range.contains(fieldRange),
                  "Phase 0 allocation %1% overflows the phase 0 region %2% for "
                  "field %3%",
                  fieldRange, phase0Range, field.isPadding() ? "(padding)" : field.source);

        posBits += field.width;
        if (field.isPadding()) continue;
        if (wroteAtLeastOneField) out << ", ";
        wroteAtLeastOneField = true;

        const le_bitrange leFieldRange = fieldRange.toOrder<Endian::Little>(phase0Range.size());
        out << field.source << ": " << leFieldRange.lo << ".." << leFieldRange.hi;
    }
    out << "}" << std::endl;

    // Write out the constant value. This value is used by the driver to
    // initialize the phase 0 data before the bits assigned to fields are given
    // their user-provided values. Having this available gives us a little more
    // flexibility when packing phase 0 fields.
    // TODO: The above isn't actually implemented, but it's planned. Right
    // now, the driver acts as if this is always set to zero.
    out << indent << "constant_value: 0" << std::endl;

    // Write out the actions block with the param order
    // TODO: This is a fake action block output in assembly to allow
    // generating context json as expected by driver. No instructions are
    // generated as,
    // 1. phase0 does not do any actual ALU operations
    // 2. This info is not needed in the context json (for now).
    // Glass does generate primitives (for model logging) which requires
    // setting ingress metadata fields as ALU ops but it is unclear if model
    // uses this info.
    out << indent << "actions:" << std::endl;
    out << ++indent << canon_name(p0->actionName) << ":" << std::endl;
    // Phase0 action handle must be unique from all other action handles. While
    // this is never used, driver expects unique handles as phase0 is
    // represented as a table construct in context.json. We assign the starting
    // handle to phase0, all other action handles start at (0x20 << 24) + 1
    // action handles is typically generated in assembler, this is special case.
    out << indent << "- handle: 0x" << hex(p0->handle) << std::endl;
    out << indent << "- p4_param_order: { ";
    wroteAtLeastOneField = false;
    for (auto &field : *packing) {
        if (field.isPadding()) continue;
        if (wroteAtLeastOneField) out << ", ";
        out << field.source << ": " << field.width;
        wroteAtLeastOneField = true;
    }
    out << " } " << std::endl;
    return out;
}
