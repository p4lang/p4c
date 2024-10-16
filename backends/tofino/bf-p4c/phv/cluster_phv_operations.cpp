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

#include "bf-p4c/phv/cluster_phv_operations.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/ir/bitrange.h"
#include "common/utils.h"
#include "lib/log.h"

const ordered_set<cstring> PHV_Field_Operations::BITWISE_OPS = {
    "set"_cs,
    "conditionally-set"_cs,
    "and"_cs,
    "or"_cs,
    "not"_cs,
    "nor"_cs,
    "andca"_cs,
    "andcb"_cs,
    "nand"_cs,
    "orca"_cs,
    "orcb"_cs,
    "xnor"_cs,
    "xor"_cs
};

const ordered_set<cstring> PHV_Field_Operations::SHIFT_OPS = {
    "shl"_cs,
    "shru"_cs,
    "shrs"_cs,
    "funnel-shift"_cs
};

const ordered_set<cstring> PHV_Field_Operations::SATURATE_OPS = {
    "saddu"_cs,
    "sadds"_cs,
    "ssubu"_cs,
    "ssubs"_cs
};

bool PHV_Field_Operations::Find_Salu_Sources::preorder(const IR::MAU::SaluAction *a) {
    visit(a->action, "action");  // just visit the action instructions
    return false;
}

bool PHV_Field_Operations::Find_Salu_Sources::preorder(const IR::Expression *e) {
    le_bitrange bits;
    if (auto *finfo = phv.field(e, &bits)) {
        if (!findContext<IR::MAU::IXBarExpression>()) {
            phv_sources[finfo][bits] = e;
            collapse_contained(phv_sources[finfo]);
        }
        return false;
    }
    return true;
}

bool PHV_Field_Operations::Find_Salu_Sources::preorder(const IR::MAU::HashDist *) {
    // Handled in a different section
    return false;
}

bool PHV_Field_Operations::Find_Salu_Sources::preorder(const IR::MAU::IXBarExpression *e) {
    for (auto ex : hash_sources)
        if (ex->equiv(*e))
            return true;
    hash_sources.push_back(e);
    return true;
}

/* Remove ranges from the map if they are contained in some other range in the map. This is largely
 * inspired from input_xbar.cpp FindSaluSources used to validate the PHV allocation. Modification
 * from one function should probably be mirrored on the other. Currently the code does not seem to
 * properly handle range overlap and assume that one of the two compared range include the other.
 * If a range have overlap with another one, both will be kept intact and the
 * max_total_operand_size might be overvalued triggering an unneeded alignment constraint.
 */
void PHV_Field_Operations::Find_Salu_Sources::collapse_contained(std::map<le_bitrange,
                                                                 const IR::Expression *> &m) {
    for (auto it = m.begin(); it != m.end();) {
        bool remove = false;
        for (auto &el : Keys(m)) {
            if (el == it->first) continue;
            if (el.contains(it->first)) {
                remove = true;
                break; }
            if (el.lo > it->first.lo) break; }
        if (remove)
            it = m.erase(it);
        else
            ++it; }
}

void PHV_Field_Operations::processSaluInst(const IR::MAU::Instruction* inst) {
    LOG4("Stateful instruction: " << inst);
    // SALU operands have the following constraints:
    //
    //  - If the SALU instruction is the same size as the container sources,
    //    eg. 8b SALU with operands in 8b containers, then the operands must be
    //    aligned
    //
    //  - If the SALU instruction is smaller than the containers holding its
    //    operands, eg. 8b SALU drawing 8b operands from 16b containers, then the
    //    operands must be allocated at bytes 0 and 1 in their respective
    //    containers (but it doesn't matter which is allocated at 0 and which at 1).
    //
    //  - SALUs have a byte mask, not a bit mask, so nothing can be packed in
    //    the same byte as an SALU operand.
    //    FIXME: This is only true when pulling in SALU inputs from a RAM
    //    search bus.  If the input is provided from hash, there is no such
    //    constraint.
    //
    // TODO: This last constraint is not implemented!

    auto* statefulAlu = findContext<IR::MAU::StatefulAlu>();
    BUG_CHECK(statefulAlu, "Found an SALU instruction not in a Stateful ALU IR node: %1%", inst);
    int sourceWidth = statefulAlu->source_width();

    Find_Salu_Sources sources(phv);
    statefulAlu->apply(sources);
    size_t max_total_operand_size = 0;
    for (auto &source : sources.phv_sources) {
        auto field = source.first;
        for (auto &range : Keys(source.second)) {
            LOG4("\tStateful ALU Field:" << field << " range:" << range);
            // Why do we round up like that?! I don't know.
            if (range.size() % 8 <= 1)
                // If the field is byte aligned or 1 bit larger than a byte aligned size,
                // then round it up to the next byte-aligned bit size.
                max_total_operand_size += (8 * ROUNDUP(range.size(), 8));
            else
                // If the field is more than 1 bit larger than a byte aligned size,
                // then it may take up one byte more than the next byte-aligned size
                // in the worst case.
                max_total_operand_size += (8 * (ROUNDUP(range.size(), 8) + 1));
            LOG4("\tStateful max_total_operand_size increased to:" << max_total_operand_size);
        }
    }

    bool is_bitwise_op = BITWISE_OPS.count(inst->name);
    if (!inst->operands.empty()) {
        for (int idx = 0; idx < int(inst->operands.size()); ++idx) {
            le_bitrange field_bits;
            PHV::Field* field = phv.field(inst->operands[idx], &field_bits);
            if (!field) continue;

            // Add details of this operation to the field object.
            field->operations().push_back({
                is_bitwise_op,
                true /* is_salu_inst */,
                inst,
                idx == 0 ? PHV::FieldAccessType::W : PHV::FieldAccessType::R,
                field_bits });

            // Require SALU operands to be placed in the bottom bits of their
            // PHV containers.  In the future, this can be handled by
            // allocating a hash table to arbitrarily swizzle the bits to get
            // them into the right place.
            //
            // Moreover, when an SALU operand is placed in a larger PHV
            // container, it must be placed such that it can be loaded into the
            // right place on the input crossbar.
            // FIXME: Again, this is only a constraint until can use the
            // hash input path.
            //
            // IXBAR byte indices
            //  8 <--  8b, 16b, 32b SALU operand 1 starts here
            //  9 <--  8b SALU operand 2 starts here
            // 10 <-- 16b SALU operand 2 starts here
            // 11
            // 12 <-- 32b SALU operand 2 starts here
            // ..
            //
            // PHV containers by size:
            //      [ X ]: X can be loaded to any byte of the IXBAR
            //     [ XY ]: X can be loaded to IXBAR index % 2
            //             Y can be loaded to IXBAR index % 2 + 1
            //   [ XYZA ]: X can be loaded to IXBAR index % 4
            //             Y can be loaded to IXBAR index % 4 + 1
            //             ...
            //
            // Therefore, if the size of the SALU operand (sourceWidth) is
            // placed in a larger container, it must start at a position that
            // corresponds to one of the two SALU operand slots in the IXBAR.
            //
            // TODO [ ARTIFICIAL CONSTRAINT ]: We require that the first
            // operand be allocated in a place that can be loaded into the
            // first SALU operand slot, and the second be allocated such that
            // it can be loaded into the second slot.  However, they could be
            // allocated in reverse.  We don't handle that kind of conditional
            // constraint (choice of two possibilities) yet.
            //
            if (max_total_operand_size <= SALU_HASH_SOURCE_LIMIT) continue;
            for (auto size : Device::phvSpec().containerSizes()) {
                if (sourceWidth <= int(size)) {
                    LOG4("Setting Field:" << field << " setStartBits(" << size << ", bitvec(0, 1)");
                    field->setStartBits(size, bitvec(0, 1));
                } else {
                    LOG4("Setting Field:" << field << " setStartBits(" << size << ", bitvec(" <<
                         int(idx * int(size)) << ", 1)");
                    field->setStartBits(size, bitvec(idx * int(size), 1)); } } }
                }
}

void PHV_Field_Operations::processInst(const IR::MAU::Instruction* inst) {
    if (inst->operands.empty())
        return;

    LOG4("Instruction: " << inst);
    le_bitrange dest_bits;
    auto* dst = phv.field(inst->operands[0], &dest_bits);
    bool alignStatefulSource = false;
    for (int idx = 0; idx < int(inst->operands.size()); ++idx) {
        if (idx > 0) {
            LOG4("  Operand " << idx << ": " << inst->operands[idx]);
            if (inst->name != "set" && inst->operands[idx]->is<IR::MAU::AttachedOutput>()) {
                LOG4("    Stateful output used as operand.");
                alignStatefulSource = true;
            }
        }
        le_bitrange field_bits;
        PHV::Field* field = phv.field(inst->operands[idx], &field_bits);
        if (!field) {
            // Each variant of subtraction must have PHV as the last operand. Constant are
            // pre-processed and transformed as "add" instruction prior to this pass.
            if ((idx + 1) == int(inst->operands.size()) &&
                (inst->name == "sub" || inst->name == "subc" || inst->name == "ssubu" ||
                 inst->name == "ssubs")) {
                // sizeInBytes and sizeInBits will be resolved as constant.
                const IR::Expression *e = inst->operands[idx];
                if (auto *sl = e->to<IR::Slice>())
                    e = sl->e0;

                if (e->is<IR::MAU::TypedPrimitive>()) {
                    if (e->to<IR::MAU::TypedPrimitive>()->name == "sizeInBytes" ||
                        e->to<IR::MAU::TypedPrimitive>()->name == "sizeInBits")
                        continue;
                }

                fatal_error("Second operand of arithmetic subtraction cannot be sourced from "
                            "action data. The second operand must either come from a packet field, "
                            "metadata or a constant. %1%", inst);
            }
            continue;
        }

        // Add details of this operation to the field object.
        bool is_bitwise_op = BITWISE_OPS.count(inst->name) > 0;
        field->operations().push_back({
            is_bitwise_op,
            false /* is_salu_inst */,
            inst,
            idx == 0 ? PHV::FieldAccessType::W : PHV::FieldAccessType::R,
            field_bits });

        // The remaining constraints only apply to non-bitwise operations.
        if (is_bitwise_op)
            continue;

        // For shift operations, the sources must be assigned no-pack.
        bool is_shift = SHIFT_OPS.count(inst->name) > 0;
        if (is_shift) {
            LOG3("Marking " << field->name << " as 'no pack' because it is a source of a shift "
                 "operation " << inst);
            field->set_solitary(PHV::SolitaryReason::ALU);
            if (inst->name == "funnel-shift") {
                field->set_exact_containers(true);
                if (field->size <= 32) {
                    field->set_no_split(true);
                } else {
                    field->set_no_holes(true);
                }
                continue;
            }
            bool no_split = false;
            // Do not allow a field to span on multiple container if the shift operation is only
            // applied to a slice and not the entire field.
            for (int op_idx = 0; op_idx < int(inst->operands.size()); ++op_idx) {
                le_bitrange op_bits;
                PHV::Field* op_field = phv.field(inst->operands[op_idx], &op_bits);
                if (op_field && (op_bits.size() != op_field->size)) {
                    no_split = true;
                    break;
                }
            }
            // Shift right signed operation must have the exact container constraint set to make
            // sure the most significant bit is located at the most significant bit of a container
            if (inst->name == "shrs") {
                if ((field_bits.size() % 8) != 0)
                    fatal_error("Operands of a signed shift right operations must have a size "
                                "aligned to a byte boundary but field %1% is %2% bits wide in: %3%",
                                field->name, field_bits.size(), inst);

                field->set_exact_containers(true);
                field->set_same_container_group(true);
            } else if (no_split) {
                field->set_no_split_at(field_bits);
            } else {
                field->set_no_holes(true);
                field->set_exact_containers(true);
                field->set_same_container_group(true);
            }
            continue;
        }

        // Apply solitary constraint on carry-based operation. If sliced, apply
        // on the slice only.  If f can't be split but is larger than 32 bits,
        // report an error.
        if (field_bits.size() > 64)
            fatal_error(
                "Operands of arithmetic operations cannot be greater than 64 bits, "
                "but field %1% is %2% bits and is involved in: %3%", field->name,
                field_bits.size(), inst);

        if (field_bits.size() > 32) {
            if (field->exact_containers() && field->size % 32 != 0) {
                fatal_error(
                    "Header fields cannot be used in wide arithmetic ops, "
                    "if field.size *mod* 32 != 0. "
                    "But field %1% with %2% bits is involved in: %3%."
                    "Please try to change fields to bit<64> or copy header fields to metadata, "
                    "apply operations on metadata, and then copy back result to header fields",
                    field->name, field_bits.size(), inst);
            } else {
                field->set_no_holes(true);
                field->set_solitary(true);
            }
            bool success = field->add_wide_arith_start_bit(field_bits.lo);
            if (!success) {
              fatal_error(
                  "Operand field bit %1% of wide arithmetic operation cannot have even and odd "
                  "container placement constraints.  Field %2% has an even alignement "
                  "constraint from: %3%", field_bits.lo, field->name, inst);
            }
            LOG3("Marking " << field->name << "[" << field_bits.lo <<
                 "] as used in wide arithmetic operation for "
                 "instruction " << inst->name << ".");

            LOG6("  field_bits = " << field_bits);
            int lo_lsb = field_bits.lo;
            int lo_len = 32;
            int hi_lsb = lo_lsb + 32;
            int hi_len = field_bits.size() - 32;
            le_bitrange lsb_slice = le_bitrange(StartLen(lo_lsb, lo_len));
            le_bitrange msb_slice = le_bitrange(StartLen(hi_lsb, hi_len));
            field->set_no_split_at(lsb_slice);
            field->set_no_split_at(msb_slice);
            LOG6("  field " << field->name << " cannot split slice " << lsb_slice);
            LOG6("  field " << field->name << " cannot split slice " << msb_slice);
        } else {
            LOG3("Marking " << field->name << " " << field_bits
                            << " as 'no split' for its use in "
                               "instruction "
                            << inst->name << ".");
            // TODO: Unify no_split and no_split_at.
            if (field_bits.size() == field->size)
                field->set_no_split(true);
            else
                field->set_no_split_at(field_bits);
        }

        // The destination of a non-bitwise instruction must be placed alone in
        // its PHV container, because carry or shift instructions might
        // overflow from writing to the destination and clobber adjacently
        // packed fields.
        //
        // The sources possibly also need to be placed alone in their PHV
        // containers because they may also influence the destination.
        //
        // For example:
        //
        // Container 1               Container 2                Container 3
        // [ 0000 XXXX XXXX 0000 ] = [ AAAA YYYY YYYY BBBB ] OP [ CCCC ZZZZ ZZZZ DDDD ]
        //
        // where the instruction in question is X = Y OP Z, e.g., X = Y + Z.
        // The result is [ ____ RRRR RRRR ____ ], where '_' is some unknown
        // value and R is the result stored in the destination in question.
        //
        //
        // Addition/subraction (regular):
        //  - Bits below Y and Z impact X when they generate a carry (i.e.,
        //    overflow/underflow of BBBB + DDDD).
        //  - Bits above Y and Z do _not_ impact X.
        //
        //  Example:
        //    Y = 0x01, Z = 0x02, B = 0xFF, D = 0x01
        //    X = Y + Z = 0x03
        //
        //    [ 01 FF ] + [ 02 01 ] = [ 04 00 ]
        //
        //
        // Saturating addition/subtraction:
        //  - Bits below Y and Z impact X when they generate a carry (i.e.,
        //    overflow/underflow of BBBB + DDDD).
        //  - Bits above Y and Z impact X by preventing saturation of X (i.e.,
        //    Y OP Z will overflow/underflow into the higher-order bits instead
        //    of saturating).
        //
        //  Example 1:
        //    Y = 0x01, Z = 0x02, B = 0xFF, D = 0x01
        //    X = Y |+| Z = 0x03
        //
        //    [ 01 FF ] |+| [ 02 01 ] = [ 04 00 ]
        //                                ^^ - X is incorrect
        //
        //  Example 2:
        //    Y = 0xF0, Z = 0x11, A = 0x00, C = 0x00
        //    X = Y |+| Z = 0xFF
        //
        //    [ 00 F0 ] |+| [ 00 11 ] = [ 01 01 ]
        //                                   ^^ - X is incorrect

        // Discussion (keep until resolved)
        // XXX/TODO: Need to restrict packing for regular arithmetic ops.
        //
        // TODO:
        // Until we add a "don't pack the lower order bits" constraint, we'll
        // have to rely on validate allocation to catch bad packings that hit
        // this corner case.  Unfortunately, it's too restrictive to not pack
        // any sources of non-bitwise instructions.
        //
        // TODO: In some circumstances, it may be possible to pack these
        // fields in the same container if enough padding is left between them.

        if (dst == field) {
            if (inst->name == "add") {
                auto src1 = inst->operands.at(1);
                auto src2 = inst->operands.at(2);
                if (*src2 == *inst->operands.at(0)) {
                    std::swap(src1, src2);
                }

                if (
                    // TODO: Fields fitting perfectly into a container could also be
                    // aligned into msb part of a larger container. Unfortunately, that
                    // introduces new packing possibilities which currently lead to
                    // regressions in a few profiles, while the benefit isn't significant.
                    Device::phvSpec().containerSizes().count((PHV::Size)field->size) == 0
                    // TODO: fallback to solitary constrain until we implement mechanism
                    // to set per-fieldslice valid container range.
                    && field_bits.hi == field->size - 1
                    && inst->operands.at(0)->equiv(*src1)
                    && src2->is<IR::Constant>())
                {
                    field->updateValidContainerRange(nw_bitrange(0, field->size-1));
                    LOG3("Setting " << field->name << " to MSB part of container");
                } else {
                    field->set_solitary(PHV::SolitaryReason::ALU);
                    LOG3("Marking " << field->name << " as 'no pack' because it is written in "
                         "non-MOVE instruction " << inst->name << ".");
                }
            } else {
                field->set_solitary(PHV::SolitaryReason::ALU);
                LOG3("Marking " << field->name << " as 'no pack' because it is written in "
                     "non-MOVE instruction " << inst->name << ".");
            }
        }

        // For non-move operations, if the source field is smaller in size than the
        // destination field, we need to set the solitary property for the source field
        // so that the missing bits (in the source compared to the destination) do not
        // contain any value that can affect the result of the operation
        if (dst && dst != field && field_bits.size() < dest_bits.size()) {
            LOG3("Marking " << field->name << " as 'no pack' because it is a source of a "
                 "non-MOVE operation to a larger field " << dst->name);
            field->set_solitary(PHV::SolitaryReason::ALU); }

        // For saturate operations, the sources must be assigned no-pack.
        auto is_saturate = SATURATE_OPS.count(inst->name);
        if (is_saturate && field != dst) {
            LOG3("Marking "  << field->name << " as 'no pack' because it is a source of a " <<
                 "saturate operation " << inst);
            field->set_solitary(PHV::SolitaryReason::ALU); } }

    if (!alignStatefulSource) return;

    // Currently, because of limitations in the compiler, all operands of a non-set operation must
    // be aligned at bit 0 in the container, if one of the operands is a stateful ALU output.
    // TODO: Add implementation for a particular slice to be in the bottom bits of its
    // container.
    for (int idx = 0; idx < int(inst->operands.size()); ++idx) {
        le_bitrange field_bits;
        PHV::Field* field = phv.field(inst->operands[idx], &field_bits);
        if (!field) continue;
        if (field_bits.size() == field->size) {
            field->set_deparsed_bottom_bits(true);
            LOG4("    Setting " << field->name << " to deparsed bottom bits.");
        }
    }
}

bool PHV_Field_Operations::preorder(const IR::MAU::Instruction *inst) {
    if (findContext<IR::MAU::SaluAction>())
        processSaluInst(inst);
    else
        processInst(inst);
    return true;
}
