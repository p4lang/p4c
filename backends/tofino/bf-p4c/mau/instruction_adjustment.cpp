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

#include "bf-p4c/mau/instruction_adjustment.h"

#include <queue>
#include <optional>
#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/common/asm_output.h"
#include "lib/indent.h"
#include "lib/log.h"

/** Splitting shift left instruction when the fields span on multiple
 *  containers. This function try to handle various corner cases relative to
 *  shifting on multiple fields using a combination of "set", "shl" and
 *  "funnel-shift" instructions. e.g.:
 *
 *  Splitting instruction:shl(ingress::hdr.mul32_64.res, ingress::tmp0_0, 20);
 *      |-->  instruction:set(ingress::hdr.mul32_64.res[15:0], 0);
 *      |-->  instruction:shl(ingress::hdr.mul32_64.res[31:16], ingress::tmp0_0[15:0], 4);
 *      |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[47:32], ingress::tmp0_0[31:16],
 *      |                              ingress::tmp0_0[15:0], 12);
 *      |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[63:48], ingress::tmp0_0[47:32],
 *                                     ingress::tmp0_0[31:16], 12);
 *
 *  Splitting "shl" instruction is required to support shifting of field larger
 *  than 32-bit. It is also useful to relaxe the constraints for other field to
 *  be split on smaller container size, e.g.:
 *
 *  bit<32> tmp0;
 *  bit<32> tmp1;
 *  tmp1 = tmp0 << 4;
 *
 *  In this example, it is possible to carry tmp0/1 on 32, 16 or 8-bit field.
 */
static void split_shl_instruction(IR::Vector<IR::MAU::Primitive> *split,
                                  std::vector<le_bitrange> &slices, const PHV::Field *field,
                                  IR::MAU::Instruction *inst) {
    std::queue<le_bitrange> slices_q;
    int container_size = 0;
    int shift_val;

    // Instruction format is "instruction:shl(dest, src, shift);"
    const IR::Constant *c = inst->operands[2]->to<IR::Constant>();
    BUG_CHECK(c != nullptr, "No shift constant found");
    shift_val = c->asInt();

    const IR::Expression* dest_expr = inst->operands[0];
    const IR::Expression* src_expr = inst->operands[1];

    for (auto slice : slices) {
        const PHV::AllocSlice &alloc_slice = field->for_bit(slice.lo);
        if (!container_size)
            container_size = alloc_slice.container().size();
        else
            BUG_CHECK(container_size == (int)alloc_slice.container().size(),
                      "Trying to funnel shift on PHV with different size");

        le_bitrange dst_range = alloc_slice.field_slice();
        const IR::Expression* slice_expr = new IR::Slice(dest_expr, dst_range.hi, dst_range.lo);
        slices_q.push(dst_range);

        // Shifting with a value greater than the container slice high bit result in zero value
        // for the entire container. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 << 40;
        //
        // Will be split as (if tmpx uses 32-bit container):
        // Splitting instruction:shl(ingress::hdr.mul32_64.res, ingress::tmp0_0, 40);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[31:0], 0);
        //     |-->  instruction:shl(ingress::hdr.mul32_64.res[63:32], ingress::tmp0_0[31:0], 8);
        if (shift_val > slice.hi) {
            const IR::Expression* zero = new IR::Constant(
                                            IR::Type_Bits::get(alloc_slice.width()), 0);
            auto* prim = new IR::MAU::Instruction("set"_cs, { slice_expr, zero });
            split->push_back(prim);
        // Shifting with a value equal to a container boundary result in a set instruction with
        // shifted slices. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 << 32;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shl(ingress::hdr.mul32_64.res, ingress::tmp0_0, 32);
        //     |-->  instruction:set(ingress::hdr.mul32_64.res[15:0], 0);
        //     |-->  instruction:set(ingress::hdr.mul32_64.res[31:16], 0);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[47:32], ingress::tmp0_0[15:0]);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[63:48], ingress::tmp0_0[31:16]);
        } else if (shift_val >= container_size && (shift_val % container_size) == 0) {
            le_bitrange src_range = slices_q.front();
            slices_q.pop();
            const IR::Expression* slice_src = new IR::Slice(src_expr, src_range.hi, src_range.lo);
            auto* prim = new IR::MAU::Instruction("set"_cs, { slice_expr, slice_src });
            split->push_back(prim);
        // First container to be shifted normally. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 << 10;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shl(ingress::hdr.mul32_64.res, ingress::tmp0_0, 10);
        // ****|-->  instruction:shl(ingress::hdr.mul32_64.res[15:0], ingress::tmp0_0[15:0], 10);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[31:16],
        //     |                              ingress::tmp0_0[31:16], ingress::tmp0_0[15:0], 6);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[47:32],
        //     |                              ingress::tmp0_0[47:32], ingress::tmp0_0[31:16], 6);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[63:48],
        //                                    ingress::tmp0_0[63:48], ingress::tmp0_0[47:32], 6);
        } else if (shift_val >= slice.lo) {
            const IR::Expression* shift_adj = new IR::Constant(shift_val % container_size);
            le_bitrange src_range = slices_q.front();
            const IR::Expression* slice_src = new IR::Slice(src_expr, src_range.hi, src_range.lo);
            auto* prim = new IR::MAU::Instruction("shl"_cs, { slice_expr, slice_src, shift_adj });
            split->push_back(prim);
        // Following container to be shifted normally. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 << 10;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shl(ingress::hdr.mul32_64.res, ingress::tmp0_0, 10);
        //     |-->  instruction:shl(ingress::hdr.mul32_64.res[15:0], ingress::tmp0_0[15:0], 10);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[31:16],
        //     |                              ingress::tmp0_0[31:16], ingress::tmp0_0[15:0], 6);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[47:32],
        //     |                              ingress::tmp0_0[47:32], ingress::tmp0_0[31:16], 6);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[63:48],
        //                                    ingress::tmp0_0[63:48], ingress::tmp0_0[47:32], 6);
        } else {
            const IR::Expression* shift_adj = new IR::Constant(container_size -
                                                               (shift_val % container_size));
            le_bitrange src_range_lo = slices_q.front();
            slices_q.pop();
            le_bitrange src_range_hi = slices_q.front();
            const IR::Expression* slice_src2 = new IR::Slice(src_expr, src_range_hi.hi,
                                                             src_range_hi.lo);
            const IR::Expression* slice_src1 = new IR::Slice(src_expr, src_range_lo.hi,
                                                             src_range_lo.lo);
            auto* prim = new IR::MAU::Instruction("funnel-shift"_cs,
                                                { slice_expr, slice_src2, slice_src1, shift_adj });
            split->push_back(prim);
        }
    }
}

/** Splitting shift right instruction when the fields span on multiple
 *  containers. This function try to handle various corner cases relative to
 *  shifting on multiple fields using a combination of "set", "shru", "shrs" and
 *  "funnel-shift" instructions. e.g.:
 *
 *  Splitting instruction:shru(ingress::hdr.mul32_64.res, ingress::tmp0_0, 20);
 *      |-->  instruction:set(ingress::hdr.mul32_64.res[63:48], 0);
 *      |-->  instruction:shru(ingress::hdr.mul32_64.res[47:32], ingress::tmp0_0[63:48], 4);
 *      |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[31:16], ingress::tmp0_0[63:48],
 *      |                              ingress::tmp0_0[47:32], 4);
 *      |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[15:0], ingress::tmp0_0[47:32],
 *                                     ingress::tmp0_0[31:16], 4);
 *
 *  Splitting "shux" instruction is required to support shifting of field larger
 *  than 32-bit. It is also useful to relaxe the constraints for other field to
 *  be split on smaller container size, e.g.:
 *
 *  bit<32> tmp0;
 *  bit<32> tmp1;
 *  tmp1 = tmp0 >> 4;
 *
 *  In this example, it is possible to carry tmp0/1 on 32, 16 or 8-bit field.
 */
static void split_shr_instruction(IR::Vector<IR::MAU::Primitive> *split,
                                  std::vector<le_bitrange> &slices, const PHV::Field *field,
                                  IR::MAU::Instruction *inst, bool signed_opcode) {
    std::queue<le_bitrange> slices_q;
    int container_size = 0;
    int shift_val;
    int high_bit;

    // Instruction format is "instruction:shrx(dest, src, shift);"
    const IR::Constant *c = inst->operands[2]->to<IR::Constant>();
    BUG_CHECK(c != nullptr, "No shift constant found");
    shift_val = c->asInt();

    const IR::Expression* dest_expr = inst->operands[0];
    const IR::Expression* src_expr = inst->operands[1];

    // Process the slices from the highest bit range to the lowest.
    for (auto slice : boost::adaptors::reverse(slices)) {
        const PHV::AllocSlice &alloc_slice = field->for_bit(slice.lo);
        if (!container_size) {
            container_size = alloc_slice.container().size();
            high_bit = slice.hi;
        } else {
            BUG_CHECK(container_size == (int)alloc_slice.container().size(),
                      "Trying to funnel shift on PHV with different size");
        }

        le_bitrange dst_range = alloc_slice.field_slice();
        const IR::Expression* slice_expr = new IR::Slice(dest_expr, dst_range.hi, dst_range.lo);
        slices_q.push(dst_range);

        // Shifting with a value greater than the container slice high bit result in zero value
        // for the entire container in case of unsigned shift right. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 >> 40;
        //
        // Will be split as (if tmpx uses 32-bit container):
        // Splitting instruction:shru(ingress::hdr.mul32_64.res, ingress::tmp0_0, 40);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[63:32], 0);
        //     |-->  instruction:shru(ingress::hdr.mul32_64.res[31:0], ingress::tmp0_0[63:32], 8);
        //
        // Signed use case is different because we have to carry the sign bit, e.g.:
        // Splitting instruction:shrs(ingress::hdr.mul32_64.res, ingress::tmp0_0, 40);
        // ****|-->  instruction:shrs(ingress::hdr.mul32_64.res[63:32], ingress::tmp0_0[63:32], 31);
        //     |-->  instruction:shrs(ingress::hdr.mul32_64.res[31:0], ingress::tmp0_0[63:32], 8);
        if (shift_val > high_bit - slice.lo) {
            if (signed_opcode) {
                const IR::Expression* shift_adj = new IR::Constant(container_size - 1);
                le_bitrange src_range = slices_q.front();
                const IR::Expression* slice_src = new IR::Slice(src_expr, src_range.hi,
                                                                src_range.lo);
                auto *prim =
                    new IR::MAU::Instruction("shrs"_cs, {slice_expr, slice_src, shift_adj});
                split->push_back(prim);
            } else {
                const IR::Expression *zero = new IR::Constant(
                                                IR::Type_Bits::get(alloc_slice.width()), 0);
                auto* prim = new IR::MAU::Instruction("set"_cs, { slice_expr, zero });
                split->push_back(prim);
            }
        // Shifting with a value equal to a container boundary result in a set instruction with
        // shifted slices. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 >> 32;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shrs(ingress::hdr.mul32_64.res, ingress::tmp0_0, 32);
        //     |-->  instruction:shrs(ingress::hdr.mul32_64.res[63:48], ingress::tmp0_0[63:48], 15);
        //     |-->  instruction:shrs(ingress::hdr.mul32_64.res[47:32], ingress::tmp0_0[63:48], 15);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[31:16], ingress::tmp0_0[63:48]);
        // ****|-->  instruction:set(ingress::hdr.mul32_64.res[15:0], ingress::tmp0_0[47:32]);
        } else if (shift_val >= container_size && (shift_val % container_size) == 0) {
            le_bitrange src_range = slices_q.front();
            slices_q.pop();
            const IR::Expression* slice_src = new IR::Slice(src_expr, src_range.hi, src_range.lo);
            auto* prim = new IR::MAU::Instruction("set"_cs, { slice_expr, slice_src });
            split->push_back(prim);
        // First container to be shifted normally. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 >> 10;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shru(ingress::hdr.mul32_64.res, ingress::tmp0_0, 10);
        // ****|-->  instruction:shru(ingress::hdr.mul32_64.res[63:48], ingress::tmp0_0[63:48], 10);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[47:32],
        //     |                              ingress::tmp0_0[63:48], ingress::tmp0_0[47:32], 10);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[31:16],
        //     |                              ingress::tmp0_0[47:32], ingress::tmp0_0[31:16], 10);
        //     |-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[15:0],
        //                                    ingress::tmp0_0[31:16], ingress::tmp0_0[15:0], 10);
        } else if (shift_val >= high_bit - slice.hi) {
            const IR::Expression* shift_adj = new IR::Constant(shift_val % container_size);
            le_bitrange src_range = slices_q.front();
            const IR::Expression* slice_src = new IR::Slice(src_expr, src_range.hi, src_range.lo);
            auto* prim = new IR::MAU::Instruction(signed_opcode ? "shrs"_cs : "shru"_cs,
                                                  { slice_expr, slice_src, shift_adj });
            split->push_back(prim);
        // Following container to be shifted normally. e.g.:
        // bit<64> tmp0 = 0;
        // bit<64> tmp1 = 0;
        // tmp1 = tmp0 >> 10;
        //
        // Will be split as (if tmpx uses 16-bit container):
        // Splitting instruction:shru(ingress::hdr.mul32_64.res, ingress::tmp0_0, 10);
        //     |-->  instruction:shru(ingress::hdr.mul32_64.res[63:48], ingress::tmp0_0[63:48], 10);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[47:32],
        //     |                              ingress::tmp0_0[63:48], ingress::tmp0_0[47:32], 10);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[31:16],
        //     |                              ingress::tmp0_0[47:32], ingress::tmp0_0[31:16], 10);
        // ****|-->  instruction:funnel-shift(ingress::hdr.mul32_64.res[15:0],
        //                                    ingress::tmp0_0[31:16], ingress::tmp0_0[15:0], 10);
        } else {
            const IR::Expression* shift_adj = new IR::Constant(shift_val % container_size);
            le_bitrange src_range_hi = slices_q.front();
            slices_q.pop();
            le_bitrange src_range_lo = slices_q.front();
            const IR::Expression* slice_src2 = new IR::Slice(src_expr, src_range_hi.hi,
                                                             src_range_hi.lo);
            const IR::Expression* slice_src1 = new IR::Slice(src_expr, src_range_lo.hi,
                                                             src_range_lo.lo);
            auto* prim = new IR::MAU::Instruction("funnel-shift"_cs,
                                                { slice_expr, slice_src2, slice_src1, shift_adj });
            split->push_back(prim);
        }
    }
}

/** AdjustShiftInstructions pass adjust shift instruction that only write to a single slice but
  * sourced from multiple containers. This is a list of various shift + slice operation and
  * their expected translation:
  *     header my_header_t {
  *         bit<32> a;
  *         bit<32> b; }
  *  ...
  *     struct ig_md_t {
  *         int<32> a;
  *         bit<32> b; }
  *  ...
  *(1)  h.my_header.a[7:0] = (ig_md.a >> 3)[15:8];
  *  ----> shrs(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[15:8], 3);
  *  -> Translated to --> funnel-shift(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[23:16],
  *                                    ingress::ig_md.a[15:8], 3);
  *
  *(2)  h.my_header.a[7:0] = ig_md.a[15:8] >> 3;
  *  ----> shru(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[15:8], 3);
  *  -> No Translation needed
  *
  *(3)  h.my_header.b[7:0] = (ig_md.b >> 3)[15:8];
  *  ----> set(ingress::hdr.my_header.b[7:0], ingress::ig_md.b[18:11]);
  *  -> No Translation needed
  *
  *(4)  h.my_header.b[7:0] = ig_md.b[15:8] >> 3;
  *  ----> shru(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[15:8], 3);
  *  -> No Translation needed
  *
  *(5) h.my_header.a[7:0] = (ig_md.a << 3)[15:8];
  *  ----> set(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[12:5]);
  *  -> No Translation needed
  *
  *(6)  h.my_header.a[7:0] = ig_md.a[15:8] << 3;
  *  ----> shl(ingress::hdr.my_header.a[7:0], ingress::ig_md.a[15:8], 3);
  *  -> No Translation needed
  *
  *(7)  h.my_header.b[7:0] = (ig_md.b << 3)[15:8];
  *  ----> set(ingress::hdr.my_header.b[7:0], ingress::ig_md.b[12:5]);
  *  -> No Translation needed
  *
  *(8)  h.my_header.b[7:0] = ig_md.b[15:8] << 3;
  *  ----> shl(ingress::hdr.my_header.b[7:0], ingress::ig_md.b[15:8], 3);
  *  -> No Translation needed
  */
const IR::Node *AdjustShiftInstructions::preorder(IR::MAU::Instruction *inst) {
    cstring opcode = inst->name;

    // Currently only support signed shift right operation since all of the other shift operation
    // working on a single slice must have been handled through slicing + deposit field.
    if (opcode != "shrs") return inst;

    le_bitrange bits;
    auto* field = phv.field(inst->operands.at(0), &bits);
    if (!field) return inst;  // error?

    int num_slices = 0;
    int dst_cont_size;
    const PHV::FieldUse use(PHV::FieldUse::WRITE);
    field->foreach_alloc(bits, findContext<IR::MAU::Table>(), &use,
                         [&](const PHV::AllocSlice& alloc) {
        num_slices++;
        dst_cont_size = alloc.container().size();
    });
    BUG_CHECK(num_slices >= 1, "No PHV slices allocated for %1%", PHV::FieldSlice(field, bits));
    if (num_slices > 1) return inst;  // This will be handled by split instruction

    // Instruction only needs to be adjusted if the destination container is smaller than the
    // source field. Otherwise, the operation will be aligned properly.
    le_bitrange src_bits;
    auto* src_field = phv.field(inst->operands.at(1), &src_bits);
    if (src_field->size <= dst_cont_size) return inst;

    // Instruction format is "instruction:shrx(dest, src, shift);"
    const IR::Constant *c = inst->operands[2]->to<IR::Constant>();
    BUG_CHECK(c != nullptr, "No shift constant found");
    int shift_val = c->asInt();

    BUG_CHECK(bits.size() == dst_cont_size,
              "Destination slice does not cover the entire destination container");

    LOG5("Adjusting signed shift right instruction: " << inst);
    IR::MAU::Instruction *adjust = nullptr;
    const IR::Expression* dest_expr = inst->operands[0];
    const IR::Expression* src_expr = inst->operands[1];

    const IR::Slice *src_slice = src_expr->to<IR::Slice>();
    BUG_CHECK(src_slice != nullptr, "No source slice found");
    unsigned low_bit = src_slice->getL();
    int offset = shift_val + low_bit;

    // Translating instruction:shrs(ingress::my_header.a[7:0], ingress::my_md.a[7:0], 24);
    // ******|-->  instruction:set(ingress::my_header.a[7:0], ingress::my_md.a[31:24]);
    if ((offset % dst_cont_size) == 0 && (offset < src_field->size)) {
        const IR::Expression *new_src_expr = new IR::Slice(src_slice->e0,
                                                           offset + dst_cont_size - 1, offset);
        adjust = new IR::MAU::Instruction("set"_cs, { dest_expr, new_src_expr });

    // Translating instruction:shrs(ingress::my_header.a[7:0], ingress::my_md.a[7:0], 25);
    // ******|-->  instruction:shrs(ingress::my_header.a[7:0], ingress::my_md.a[31:24], 1);
    } else if (offset + dst_cont_size > src_field->size) {
        shift_val = (offset + dst_cont_size) - src_field->size;
        if (shift_val >= dst_cont_size)
            shift_val = dst_cont_size - 1;

        const IR::Expression* shift_adj = new IR::Constant(shift_val);
        const IR::Expression *new_src_expr = new IR::Slice(src_slice->e0, src_field->size - 1,
                                                           src_field->size - dst_cont_size);
        adjust = new IR::MAU::Instruction("shrs"_cs, { dest_expr, new_src_expr, shift_adj });

    // Translating instruction:shrs(ingress::my_header.a[7:0], ingress::my_md.a[7:0], 9);
    // ******|-->  instruction:funnel-shift(ingress::my_header.a[7:0], ingress::my_md.a[23:16],
    //                                      ingress::my_md.a[15:8], 1);
    } else {
        const IR::Expression* shift_adj = new IR::Constant(offset % dst_cont_size);
        int container_offset = offset / dst_cont_size;
        int high_bit = ((container_offset + 1) * dst_cont_size) - 1;
        int low_bit = container_offset * dst_cont_size;

        const IR::Expression* slice_src1 = new IR::Slice(src_slice->e0, high_bit, low_bit);
        const IR::Expression* slice_src2 = new IR::Slice(src_slice->e0, high_bit + dst_cont_size,
                                                         low_bit + dst_cont_size);
        adjust = new IR::MAU::Instruction("funnel-shift"_cs,
                                          { dest_expr, slice_src2, slice_src1, shift_adj });
    }

    LOG5("Translated instruction: " << adjust);
    return adjust;
}

/** SplitInstructions */
const IR::Node *SplitInstructions::preorder(IR::MAU::Instruction *inst) {
    le_bitrange bits;
    auto* field = phv.field(inst->operands.at(0), &bits);
    if (!field) return inst;  // error?

    std::vector<le_bitrange> slices;
    PHV::FieldUse use(PHV::FieldUse::WRITE);
    field->foreach_alloc(bits, findContext<IR::MAU::Table>(), &use,
                         [&](const PHV::AllocSlice& alloc) {
        slices.push_back(alloc.field_slice());
    });
    BUG_CHECK(slices.size() >= 1, "No PHV slices allocated for %1%, instruction: %2%.",
              PHV::FieldSlice(field, bits), inst);
    if (slices.size() == 1) return inst;  // nothing to split

    LOG5("Splitting instruction:" << inst);

    // Check if this is an operator that cannot be split
    cstring opcode = inst->name;
    BUG_CHECK(opcode != "saddu" && opcode != "sadds" &&
            opcode != "ssubu" && opcode != "ssubs",
            "Saturating arithmetic operations cannot be split");

    auto split = new IR::Vector<IR::MAU::Primitive>();

    if (opcode == "shl") {
        split_shl_instruction(split, slices, field, inst);
    } else if (opcode == "shrs" || opcode == "shru") {
        split_shr_instruction(split, slices, field, inst, opcode == "shrs");
    } else {
        bool check_pairing = false;
        for (auto slice : slices) {
            auto *n = inst->clone();
            n->name = opcode;
            for (auto &operand : n->operands)
                operand = MakeSlice(operand, slice.lo - bits.lo, slice.hi - bits.lo);
            split->push_back(n);
            // FIXME -- addc/subc only work if there are exactly 2 slices and they're in
            // an even-odd pair of PHVs.  Check for failure to meet that constraint and
            // give an error?
            if (opcode == "add" || opcode == "sub") {
                opcode += "c";
                check_pairing = true; } }
        if (check_pairing) {
            BUG_CHECK(slices.size() == 2, "PHV pairing failure for wide %s", inst->name);
            auto &s1 = field->for_bit(slices[0].lo);
            auto &s2 = field->for_bit(slices[1].lo);
            BUG_CHECK(s1.container().index() + 1 == s2.container().index() &&
                      (s1.container().index() & 1) == 0,
                      "PHV alloc failed to put wide %s into even/odd PHV pair", inst->name); }
    }

    for (auto &split_inst : *split)
        LOG5("Splitted instruction:" << split_inst);

    return split;
}

/** ConvertConstantsToActionData */

/** The purpose of this pass is to either convert all constants that are necessarily supposed
 *  to be action data, to action data, or if the action formats were decided before PHV
 *  allocation was known, throw a Backtrack exception back to TableLayout in order to determine
 *  the best allocation.
 *
 *  If a container is determined to need conversion, then all of the constants are converted into
 *  action data for that container.  The reasons constants are converted are detailed in the
 *  action_analysis pass, but summarized are restricted from a load_const, and a src2 limitation
 *  on all instructions.
 */
const IR::MAU::Action *ConstantsToActionData::preorder(IR::MAU::Action *act) {
    LOG1("ConstantsToActionData preorder on action: " << act);
    container_actions_map.clear();
    constant_containers.clear();
    auto tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, true, true, tbl, red_info);
    aa.set_container_actions_map(&container_actions_map);
    act->apply(aa);

    bool proceed = false;
    for (auto &container_action_entry : container_actions_map) {
        auto container = container_action_entry.first;
        auto &cont_action = container_action_entry.second;
        if (cont_action.convert_constant_to_actiondata()) {
            proceed = true;
            constant_containers.insert(container);
            LOG3("  adding constant container : " << container);
        }
    }
    if (!proceed) {
        prune();
        return act;
    }

    action_name = act->name;
    return act;
}


const IR::Node *ConstantsToActionData::preorder(IR::Node *node) {
    visitOnce();
    return node;
}

const IR::MAU::Instruction *ConstantsToActionData::preorder(IR::MAU::Instruction *instr) {
    LOG1("ConstantsToActionData preorder on instruction : " << instr);
    write_found = false;
    has_constant = false;
    if (auto act = findContext<IR::MAU::Action>()) {
        constant_rename_key.action_name = act->name;
        LOG3("  Setting constant_rename_key action name : : " << constant_rename_key.action_name);
    }
    return instr;
}

const IR::MAU::ActionArg *ConstantsToActionData::preorder(IR::MAU::ActionArg *arg) {
    return arg;
}

const IR::MAU::Primitive *ConstantsToActionData::preorder(IR::MAU::Primitive *prim) {
    prune();
    return prim;
}

const IR::Constant *ConstantsToActionData::preorder(IR::Constant *constant) {
    LOG1("ConstantsToActionData preorder on constant : " << constant);
    has_constant = true;
    unsigned constant_value = constant->value < 0 ?
        static_cast<unsigned>(static_cast<int>(constant->value)) :
        static_cast<unsigned>(constant->value);
    ActionData::Parameter *param = new ActionData::Constant(constant_value,
                                                            constant->type->width_bits());
    constant_rename_key.param = param;
    LOG3("  Setting constant_rename_key param: : " << constant_rename_key.param);
    return constant;
}

void ConstantsToActionData::analyze_phv_field(IR::Expression *expr) {
    le_bitrange bits;
    auto *field = phv.field(expr, &bits);

    if (field == nullptr)
        return;

    LOG3("  analyzing phv field " << field);
    if (isWrite()) {
        LOG3("  analyzing phv field for writes ");
        if (write_found)
            BUG("Multiple writes found within a single field instruction");

        int write_count = 0;
        le_bitrange container_bits;
        PHV::Container container;
        PHV::FieldUse use(PHV::FieldUse::WRITE);
        field->foreach_alloc(bits, findContext<IR::MAU::Table>(), &use,
                             [&](const PHV::AllocSlice &alloc) {
            write_count++;
            container_bits = alloc.container_slice();
            container = alloc.container();
            LOG3("  writing " << container_bits << " on container " << container);
        });

        if (write_count != 1)
            BUG("Splitting of writes did not work in ConstantsToActionData");

        constant_rename_key.container = container;
        constant_rename_key.phv_bits = container_bits;
        write_found = true;
    }
}

const IR::Slice *ConstantsToActionData::preorder(IR::Slice *sl) {
    LOG1(" ConstantsToActionData preorder on Slice " << sl);
    if (phv.field(sl))
        analyze_phv_field(sl);

    prune();
    return sl;
}

const IR::Expression *ConstantsToActionData::preorder(IR::Expression *expr) {
    if (!findContext<IR::MAU::Instruction>()) {
        prune();
        return expr;
    }

    if (phv.field(expr)) {
        prune();
        analyze_phv_field(expr);
    }
    return expr;
}

const IR::MAU::AttachedOutput *ConstantsToActionData::preorder(IR::MAU::AttachedOutput *ao) {
    prune();
    return ao;
}

const IR::MAU::StatefulAlu *ConstantsToActionData::preorder(IR::MAU::StatefulAlu *salu) {
    prune();
    return salu;
}

const IR::MAU::HashDist *ConstantsToActionData::preorder(IR::MAU::HashDist *hd) {
    prune();
    return hd;
}

/** Replace any constant in these particular instructions with the an IR::MAU::ActionDataConstant
 */
const IR::MAU::Instruction *ConstantsToActionData::postorder(IR::MAU::Instruction *instr) {
    LOG1(" ConstantsToActionData postorder on instruction " << instr);
    if (!write_found)
        BUG("No write found in an instruction in ConstantsToActionData?");

    if (constant_containers.find(constant_rename_key.container) == constant_containers.end())
        return instr;
    if (!has_constant)
        return instr;

    LOG1("   instruction has constant : " << has_constant);

    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for instruction - %1%", instr);

    auto &action_format = tbl->resources->action_format;

    auto *alu_parameter = action_format.find_param_alloc(constant_rename_key, nullptr);
    bool constant_found = alu_parameter != nullptr;

    if (constant_found != has_constant)
        BUG("Constant lookup does not match the ActionFormat");

    if (!constant_found)
        return instr;

    auto alias = alu_parameter->param->to<ActionData::Constant>()->alias();

    for (size_t i = 0; i < instr->operands.size(); i++) {
        const IR::Constant *c = instr->operands[i]->to<IR::Constant>();
        if (c == nullptr)
            continue;
        int size = c->type->width_bits();
        auto *adc = new IR::MAU::ActionDataConstant(IR::Type::Bits::get(size), alias, c);
        instr->operands[i] = adc;
    }
    return instr;
}

const IR::MAU::Action *ConstantsToActionData::postorder(IR::MAU::Action *act) {
    LOG1(" ConstantsToActionData postorder on action " << act);
    return act;
}


/**
 * Certain Expressions (currently only IR::Constants) because of their associated uses in
 * ALU Operations with HashDists as well, must be converted to HashDists.  (At some point,
 * the immediate_adr_default could be used to generate constants rather than Hash, as this
 * is excessive resources.
 *
 * Currently the instruction adjustment cannot work with these default constants
 */
const IR::MAU::Action *ExpressionsToHash::preorder(IR::MAU::Action *act) {
    LOG5("ExpressionsToHash preorder on action: " << act);
    container_actions_map.clear();
    expr_to_hash_containers.clear();
    auto tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, true, true, tbl, red_info);
    aa.set_container_actions_map(&container_actions_map);
    act->apply(aa);

    for (auto &container_action_entry : container_actions_map) {
        auto container = container_action_entry.first;
        auto &cont_action = container_action_entry.second;
        if (cont_action.convert_constant_to_hash()) {
            expr_to_hash_containers.insert(container);
        }
    }

    if (expr_to_hash_containers.empty())
        prune();
    return act;
}

const IR::MAU::Instruction *ExpressionsToHash::preorder(IR::MAU::Instruction *instr) {
    prune();
    le_bitrange bits;
    auto write_expr = phv.field(instr->operands[0], &bits);
    PHV::Container container;
    ActionData::UniqueLocationKey expr_lookup;

    auto act = findContext<IR::MAU::Action>();
    BUG_CHECK(act != nullptr, "No associated action found for instruction - %1%", instr);

    expr_lookup.action_name = act->name;

    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for instruction - %1%", instr);

    PHV::FieldUse use(PHV::FieldUse::WRITE);
    int write_count = 0;
    write_expr->foreach_alloc(bits, tbl, &use, [&](const PHV::AllocSlice &alloc) {
        write_count++;
        expr_lookup.phv_bits = alloc.container_slice();
        expr_lookup.container = alloc.container();
    });

    BUG_CHECK(write_count == 1, "An expression writes to more than one container position");

    if (expr_to_hash_containers.count(expr_lookup.container) == 0)
        return instr;

    IR::MAU::Instruction *rv = new IR::MAU::Instruction(instr->srcInfo, instr->name);
    rv->operands.push_back(instr->operands[0]);

    auto &action_format = tbl->resources->action_format;

    for (size_t i = 1; i < instr->operands.size(); i++) {
        expr_lookup.param = nullptr;

        auto operand = instr->operands[i];
        const IR::Expression *rv_operand = nullptr;
        // Build a hash from a single constant
        if (auto con = operand->to<IR::Constant>()) {
            P4HashFunction func;
            func.inputs.push_back(con);
            func.algorithm = IR::MAU::HashFunction::identity();
            func.hash_bits = { 0, con->type->width_bits() - 1 };
            ActionData::Hash *param = new ActionData::Hash(func);
            expr_lookup.param = param;
            auto alu_parameter = action_format.find_param_alloc(expr_lookup, nullptr);
            BUG_CHECK(alu_parameter != nullptr, "%1% Constant in instruction has not correctly "
                   "been converted to hash");
            auto *hge = new IR::MAU::HashGenExpression(con->srcInfo, con->type, con,
                                                       IR::MAU::HashFunction::identity());
            auto *hd = new IR::MAU::HashDist(hge->srcInfo, hge->type, hge);
            rv_operand = hd;
        } else {
            rv_operand = operand;
        }
        rv->operands.push_back(rv_operand);
    }
    return rv;
}


/** Merge Instructions */

/** Run an analysis on the instructions to determine which instructions should be merged.  The
 *  merged instructions must be initially removed, and then added back as a single instruction
 *  over a container
 */
const IR::MAU::Action *MergeInstructions::preorder(IR::MAU::Action *act) {
    Log::TempIndent indent;
    LOG5("MergeInstructions preorder on action: " << act << indent);
    container_actions_map.clear();
    merged_fields.clear();
    auto tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, true, true, tbl, red_info);
    aa.set_container_actions_map(&container_actions_map);
    aa.set_verbose();
    act->apply(aa);

    unsigned allowed_errors = ActionAnalysis::ContainerAction::PARTIAL_OVERWRITE
                            | ActionAnalysis::ContainerAction::REFORMAT_CONSTANT
                            | ActionAnalysis::ContainerAction::UNRESOLVED_REPEATED_ACTION_DATA
                            | ActionAnalysis::ContainerAction::IMPOSSIBLE_ALIGNMENT
                            | ActionAnalysis::ContainerAction::ILLEGAL_OVERWRITE;
    unsigned error_mask = ~allowed_errors;

    for (auto &container_action : container_actions_map) {
        auto container = container_action.first;
        auto &cont_action = container_action.second;
        LOG5(cont_action << ", error_mask: " << std::hex << error_mask << std::dec);
        if ((cont_action.error_code & error_mask) != 0) continue;
        LOG5(" Container Action ops: " << cont_action.operands()
                << ", alignment_counts: " << cont_action.alignment_counts());
        if (cont_action.operands() == cont_action.alignment_counts()) {
            if (!cont_action.convert_instr_to_deposit_field
                && !cont_action.convert_instr_to_bitmasked_set
                && !cont_action.convert_instr_to_byte_rotate_merge
                && !cont_action.is_total_overwrite_possible()
                && !cont_action.adi.specialities.getbit(ActionAnalysis::ActionParam::HASH_DIST)
                && !cont_action.adi.specialities.getbit(ActionAnalysis::ActionParam::RANDOM)
                && !cont_action.adi.specialities.getbit(ActionAnalysis::ActionParam::METER_ALU)
                && (cont_action.error_code & ~error_mask) == 0)
                continue;
        // Currently skip unresolved ActionAnalysis issues
        }
        merged_fields.insert(container);
        LOG4("  Adding merged field: " << container);
    }

    if (merged_fields.empty())
        prune();

    return act;
}

const IR::Node *MergeInstructions::preorder(IR::Node *node) {
    visitOnce();
    return node;
}

const IR::MAU::Instruction *MergeInstructions::preorder(IR::MAU::Instruction *instr) {
    LOG5("MergeInstructions preorder on Instruction: " << instr);
    merged_location = merged_fields.end();
    write_found = false;
    if (instr->name == "sadds" || instr->name == "saddu") {
        saturationArith = true;
    }
    return instr;
}

void MergeInstructions::analyze_phv_field(IR::Expression *expr) {
    le_bitrange bits;
    auto *field = phv.field(expr, &bits);
    if (field != nullptr && isWrite()) {
        if (merged_location != merged_fields.end())
            BUG("More than one write within an instruction");

        PHV::FieldUse use(PHV::FieldUse::WRITE);
        PHV::Container cntr = PHV::Container();
        field->foreach_alloc(bits, findContext<IR::MAU::Table>(), &use,
                             [&](const PHV::AllocSlice &alloc) {
            if (cntr == PHV::Container())
                cntr = alloc.container();
            else
                BUG_CHECK((cntr == alloc.container()),
                          "Instruction on field %s not a single container instruction",
                          field->name);
            merged_location = merged_fields.find(cntr);
            write_found = true;

            if (saturationArith && cntr.size() != static_cast<size_t>(field->size)) {
                BUG("Destination of saturation add was allocated to bigger container than the "
                    "field itself, which would cause saturation to produce wrong results. You can "
                    "try constraining source operand with @pa_container_size to achieve correct "
                    "container allocation on both source and destination.");
            }
        });
    }
}

const IR::Slice *MergeInstructions::preorder(IR::Slice *sl) {
    if (phv.field(sl))
        analyze_phv_field(sl);
    prune();
    return sl;
}

/** Mark instructions that have a write corresponding to the expression being removed
 */
const IR::Expression *MergeInstructions::preorder(IR::Expression *expr) {
    LOG5("MergeInstructions preorder on expression: " << *expr);
    if (!findContext<IR::MAU::Instruction>()) {
        prune();
        return expr;
    }

    if (phv.field(expr))
        analyze_phv_field(expr);
    prune();
    return expr;
}

const IR::MAU::ActionArg *MergeInstructions::preorder(IR::MAU::ActionArg *aa) {
    prune();
    return aa;
}

const IR::MAU::ActionDataConstant *MergeInstructions::preorder(IR::MAU::ActionDataConstant *adc) {
    prune();
    return adc;
}

const IR::Constant *MergeInstructions::preorder(IR::Constant *cst) {
    prune();
    return cst;
}

const IR::MAU::Primitive *MergeInstructions::preorder(IR::MAU::Primitive *prim) {
    prune();
    return prim;
}

const IR::MAU::AttachedOutput *MergeInstructions::preorder(IR::MAU::AttachedOutput *ao) {
    prune();
    return ao;
}

const IR::MAU::StatefulAlu *MergeInstructions::preorder(IR::MAU::StatefulAlu *salu) {
    prune();
    return salu;
}

const IR::MAU::HashDist *MergeInstructions::preorder(IR::MAU::HashDist *hd) {
    prune();
    return hd;
}

/** If marked for a merge, remove the original instruction to be added back later
 */
const IR::MAU::Instruction *MergeInstructions::postorder(IR::MAU::Instruction *instr) {
    LOG5("MergeInstructions::postorder on Instruction: " << instr);
    saturationArith = false;

    if (!write_found)
        BUG("No write found within the instruction");

    if (merged_location == merged_fields.end()) {
        return instr;
    } else {
        return nullptr;
    }
}

/** Merge the Expressions as a MultiOperand, and then set the operands of these instructions
 *  as a multi-operand
 */
const IR::MAU::Action *MergeInstructions::postorder(IR::MAU::Action *act) {
    Log::TempIndent indent;
    LOG5("MergeInstructions::postorder on Action : " << act->name << indent);
    if (merged_fields.empty()) {
        LOG5("No merged fields");
        return act;
    }

    for (auto &container_action_info : container_actions_map) {
        auto container = container_action_info.first;
        auto &cont_action = container_action_info.second;
        if (!merged_fields.count(container)) continue;
        act->action.push_back(build_merge_instruction(container, cont_action));
        LOG5("Merged instr: " << *(act->action.rbegin()));
    }
    return act;
}

/** Given that a constant will only appear once, this will find the IR::Constant node within
 *  the field actions.  Thus the size of the IR node is still maintained.
 */
const IR::Constant *MergeInstructions::find_field_action_constant(
         ActionAnalysis::ContainerAction &cont_action) {
    for (auto &fa : cont_action.field_actions) {
        for (auto read : fa.reads) {
            if (read.type == ActionAnalysis::ActionParam::CONSTANT) {
                BUG_CHECK(read.expr->is<IR::Constant>(), "Value incorrectly saved as a constant");
                return read.expr->to<IR::Constant>();
            }
        }
    }
    BUG("Should never reach this point in find_field_action_constant");
    return nullptr;
}

/**
 * Convert one or many HashDist operands to a single Hash Dist object or Slice of a HashDist
 * object with its units set.
 *
 * The units have to both be coordinated through the action format (in order to know which
 * immed_lo/immed_hi to use, as well as the input_xbar alloc, in order to understand which
 * unit coordinates to hash_dist lo/hash_dist hi
 */
const IR::Expression * MergeInstructions::fill_out_hash_operand(PHV::Container container,
        ActionAnalysis::ContainerAction &cont_action) {
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No table found while building action data for hash operands");

    auto &adi = cont_action.adi;
    BUG_CHECK(adi.specialities.getbit(ActionAnalysis::ActionParam::HASH_DIST) &&
              adi.specialities.popcount() == 1,
              "Can only create hash dist from hash dist associated objects");

    bitvec op_bits_used_bv = adi.alignment.read_bits();
    bitvec immed_bits_used_bv = op_bits_used_bv << (adi.start * 8);
    auto &ixbSpec = Device::ixbarSpec();

    // Find out which sections of immediate sections and then coordinate to these to the
    // hash dist sections
    bitvec hash_dist_units_used;
    for (int i = 0; i < 2; i++) {
        if (!immed_bits_used_bv.getslice(i * ixbSpec.hashDistBits(),
                                         ixbSpec.hashDistBits()).empty())
            hash_dist_units_used.setbit(i);
    }

    BUG_CHECK(!hash_dist_units_used.empty(), "Hash Dist in %s has no allocation", cont_action);

    IR::Vector<IR::Expression> hash_dist_parts;
    for (auto &fa : cont_action.field_actions) {
        for (auto read : fa.reads) {
            if (read.speciality != ActionAnalysis::ActionParam::HASH_DIST)
                continue;
            hash_dist_parts.push_back(read.expr);
        }
    }

    IR::ListExpression *le = new IR::ListExpression(hash_dist_parts);
    auto type = IR::Type::Bits::get(hash_dist_units_used.popcount() * ixbSpec.hashDistBits());
    auto *hd = new IR::MAU::HashDist(tbl->srcInfo, type, le);

    auto tbl_hash_dists = tbl->resources->hash_dist_immed_units();
    for (auto bit : hash_dist_units_used) {
        hd->units.push_back(tbl_hash_dists.at(bit));
    }

    int wrapped_lo = 0;  int wrapped_hi = 0;
    // Wrapping the slice in the function outside
    if (adi.alignment.is_wrapped_shift(container, &wrapped_lo, &wrapped_hi)) {
        return hd;
    }

    BUG_CHECK(immed_bits_used_bv.is_contiguous(), "Hash Dist object must be contiguous, if it "
          "not a Wrapped Sliced");
    // If a single hash dist object is used, only the 16 bit slices are necessary, so start
    // at the first position
    le_bitrange immed_bits_used = { immed_bits_used_bv.min().index(),
                                    immed_bits_used_bv.max().index() };
    int hash_dist_bits_shift = (immed_bits_used.lo / ixbSpec.hashDistBits());
    hash_dist_bits_shift *= ixbSpec.hashDistBits();
    le_bitrange hash_dist_bits_used = immed_bits_used.shiftedByBits(-1 * hash_dist_bits_shift);
    return MakeSlice(hd, hash_dist_bits_used.lo, hash_dist_bits_used.hi);
}


/**
 * The RNG_unit is assigned in this particular function (coordinated through ActionAnalysis)
 * and the rng allocation in the action data bus
 */
const IR::Expression *MergeInstructions::fill_out_rand_operand(PHV::Container container,
        ActionAnalysis::ContainerAction &cont_action) {
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No table found for building action data for random operands");

    auto &adi = cont_action.adi;
    BUG_CHECK(adi.specialities.getbit(ActionAnalysis::ActionParam::RANDOM) &&
              adi.specialities.popcount() == 1,
              "Can only create random number from random number associated objects");

    int unit = tbl->resources->rng_unit();
    auto *rn = new IR::MAU::RandomNumber(tbl->srcInfo,
                                          IR::Type::Bits::get(ActionData::Format::IMMEDIATE_BITS),
                                          "hw_rng"_cs);
    rn->rng_unit = unit;

    int wrapped_lo = 0;  int wrapped_hi = 0;
    // Wrapping the slice in the function outside
    if (adi.alignment.is_wrapped_shift(container, &wrapped_lo, &wrapped_hi)) {
        return rn;
    }

    bitvec op_bits_used_bv = adi.alignment.read_bits();
    BUG_CHECK(op_bits_used_bv.is_contiguous(), "Random Number must be contiguous if it is not a "
        "wrapped slice");
    le_bitrange op_bits_used = { op_bits_used_bv.min().index(), op_bits_used_bv.max().index() };
    le_bitrange immed_bits_used = op_bits_used.shiftedByBits(adi.start * 8);
    return MakeSlice(rn, immed_bits_used.lo, immed_bits_used.hi);
}

/** In order to keep the IR holding the fields that are contained within the container, this
 *  holds every single IR field within these individual container.  It walks over the reads of
 *  field actions within the container action, and adds them to the list.  Currently we don't
 *  use these fields in any way after this.  However, we may at some point.
 */
void MergeInstructions::fill_out_read_multi_operand(ActionAnalysis::ContainerAction &cont_action,
        ActionAnalysis::ActionParam::type_t type, cstring match_name,
        IR::MAU::MultiOperand *mo) {
    for (auto &fa : cont_action.field_actions) {
         for (auto read : fa.reads) {
             if (read.type != type) continue;
             if (type == ActionAnalysis::ActionParam::ACTIONDATA) {
                 mo->push_back(read.expr);
             } else if (type == ActionAnalysis::ActionParam::PHV) {
                le_bitrange bits;
                auto *field = phv.field(read.expr, &bits);
                int split_count = 0;
                PHV::FieldUse use(PHV::FieldUse::READ);
                field->foreach_alloc(bits, cont_action.table_context, &use,
                                     [&](const PHV::AllocSlice &alloc) {
                    split_count++;
                    if (alloc.container().toString() != match_name)
                       return;
                    const IR::Expression* read_mo_expr = read.expr;
                    if (alloc.width() != read.size()) {
                        int start = alloc.field_slice().lo - bits.lo;
                        read_mo_expr = MakeSlice(read.expr, start, start + alloc.width() - 1);
                    }
                    mo->push_back(read_mo_expr);
                });
             }
         }
    }
}

/** Fills out the IR::MAU::MultiOperand will all of the underlying fields that are part of a
 *  write.
 */
void MergeInstructions::fill_out_write_multi_operand(ActionAnalysis::ContainerAction &cont_action,
        IR::MAU::MultiOperand *mo) {
    for (auto &fa : cont_action.field_actions) {
        mo->push_back(fa.write.expr);
    }
}


/** The purpose of this is to convert any full container instruction destination to the container
 *  in order for the container to be the correct size.  The assembler will only parse full
 *  container instruction if the destination is the correct size.  This is based on the
 *  verify_overwritten check in ActionAnalysis in order to determine that a partial overwrite of
 *  the container is actually valid, due to the rest of the container being unoccupied.
 */
IR::MAU::Instruction *MergeInstructions::dest_slice_to_container(PHV::Container container,
        ActionAnalysis::ContainerAction &cont_action) {
    LOG3("Convert destination slice to container");
    BUG_CHECK(cont_action.field_actions.size() == 1, "Can only call this function on an operation "
                                                     "that has one field action");
    IR::MAU::Instruction *rv = new IR::MAU::Instruction(cont_action.name);
    IR::Vector<IR::Expression> components;
    auto *dst_mo = new IR::MAU::MultiOperand(components, container.toString(), true);
    fill_out_write_multi_operand(cont_action, dst_mo);
    rv->operands.push_back(dst_mo);
    for (auto &read : cont_action.field_actions[0].reads) {
        rv->operands.push_back(read.expr);
    }
    LOG3("  New instruction : " << rv);
    return rv;
}

void MergeInstructions::build_actiondata_source(ActionAnalysis::ContainerAction &cont_action,
        const IR::Expression **src1_p, bitvec &src1_writebits, ByteRotateMergeInfo &brm_info,
        PHV::Container container) {
    Log::TempIndent indent;
    LOG5("Building action data source " << indent);
    IR::Vector<IR::Expression> components;
    auto &adi = cont_action.adi;
    if (adi.specialities.getbit(ActionAnalysis::ActionParam::HASH_DIST)) {
        *src1_p = fill_out_hash_operand(container, cont_action);
        src1_writebits = adi.alignment.write_bits();
        LOG5("hashdist " << cont_action.name << "  writebits:" << src1_writebits);
    } else if (adi.specialities.getbit(ActionAnalysis::ActionParam::RANDOM)) {
        *src1_p = fill_out_rand_operand(container, cont_action);
        src1_writebits = adi.alignment.write_bits();
        LOG5("random " << cont_action.name << "  writebits:" << src1_writebits);
    } else if (cont_action.ad_renamed()) {
        auto mo = new IR::MAU::MultiOperand(components, adi.action_data_name, false);
        fill_out_read_multi_operand(cont_action, ActionAnalysis::ActionParam::ACTIONDATA,
                                    adi.action_data_name, mo);
        *src1_p = mo;
        src1_writebits = adi.alignment.write_bits();
        LOG5("multiOp " << adi.action_data_name << "  writebits:" << src1_writebits);
    } else {
        const IR::Expression *prv_expr = nullptr;
        bitvec read_bits;
        for (auto &field_action : cont_action.field_actions) {
            for (auto &read : field_action.reads) {
                if (read.type != ActionAnalysis::ActionParam::ACTIONDATA)
                    continue;
                if (read.is_conditional)
                    continue;

                src1_writebits = adi.alignment.write_bits();
                BUG_CHECK(!read_bits.getrange(read.range().lo, read.range().size()),
                    "Overlapping AD read params ...?");
                read_bits.setrange(read.range().lo, read.range().size());
                *src1_p = read.expr;
                LOG5("field " << field_action.name << " src1_p:" << **src1_p << "  writebits:" <<
                     src1_writebits << "  readbits: " << read_bits);

                if (auto *slc = (*src1_p)->to<IR::Slice>()) {
                    // Store first source AD expression
                    if (prv_expr == nullptr) {
                        // Store first source AD expression
                        prv_expr = slc->e0;
                    } else {
                        // Compare first AD expression with current AD expression
                        BUG_CHECK((prv_expr == slc->e0), "Slices of multiple AD sources?");
                        BUG_CHECK((read_bits.is_contiguous()),
                                  "Non contiguous slices of AD sources?");
                    }
                    // Update returned source expression as merged slice of multiple source slices
                    *src1_p = new IR::Slice(prv_expr, (read_bits.ffs() + read_bits.popcount() - 1),
                                            read_bits.ffs());
                } else {
                    if (prv_expr == nullptr) {
                        // Store first AD expression
                        prv_expr = *src1_p;
                    } else {
                        // Compare first AD expression with current AD expression
                        BUG_CHECK(prv_expr == *src1_p, "Multiple AD sources?");
                        BUG_CHECK((read_bits.is_contiguous()),
                                  "Non contiguous AD sources?");
                    }
                }
           }
        }
    }

    int wrapped_lo = 0;  int wrapped_hi = 0;
    if (cont_action.convert_instr_to_byte_rotate_merge) {
        brm_info.src1_shift = adi.alignment.right_shift / 8;
        brm_info.src1_byte_mask = adi.alignment.byte_rotate_merge_byte_mask(container);
    } else if (!cont_action.convert_instr_to_bitmasked_set && adi.alignment.contiguous() &&
               adi.alignment.is_wrapped_shift(container, &wrapped_lo, &wrapped_hi)) {
        // The alias begins at the first bit used in the action bus slot
        wrapped_lo -= adi.alignment.direct_read_bits.min().index();
        wrapped_hi -= adi.alignment.direct_read_bits.min().index();
        *src1_p = MakeWrappedSlice(*src1_p, wrapped_lo, wrapped_hi, container.size());
    }
}

void MergeInstructions::build_phv_source(ActionAnalysis::ContainerAction &cont_action,
        const IR::Expression **src1_p, const IR::Expression **src2_p, bitvec &src1_writebits,
        bitvec &src2_writebits, ByteRotateMergeInfo &brm_info, PHV::Container container) {
    Log::TempIndent indent;
    LOG5("Building PHV Source on container " << container << indent);
    IR::Vector<IR::Expression> components;
    for (auto &phv_ta : cont_action.phv_alignment) {
        auto read_container = phv_ta.first;
        auto read_alignment = phv_ta.second;
        LOG5("PHV Align: " << read_container << ":" << read_alignment);
        if (read_alignment.is_src1) {
            auto mo = new IR::MAU::MultiOperand(components, read_container.toString(), true);
            fill_out_read_multi_operand(cont_action, ActionAnalysis::ActionParam::PHV,
                                        read_container.toString(), mo);
            *src1_p = mo;
            src1_writebits = read_alignment.write_bits();
            bitvec src1_read_bits = read_alignment.read_bits();
            int wrapped_lo = 0;  int wrapped_hi = 0;
            if (cont_action.convert_instr_to_byte_rotate_merge) {
                brm_info.src1_shift = read_alignment.right_shift / 8;
                brm_info.src1_byte_mask = read_alignment.byte_rotate_merge_byte_mask(container);
            } else if (read_alignment.is_wrapped_shift(container, &wrapped_lo, &wrapped_hi)) {
                *src1_p = MakeWrappedSlice(*src1_p, wrapped_lo, wrapped_hi, container.size());
            } else if (src1_read_bits.popcount() != static_cast<int>(read_container.size())) {
                if (src1_read_bits.is_contiguous()) {
                    *src1_p = MakeSlice(*src1_p, src1_read_bits.min().index(),
                                        src1_read_bits.max().index());
                }
            }
            LOG5("Src1: " << *src1_p);
        } else {
            auto mo = new IR::MAU::MultiOperand(components, read_container.toString(), true);
            fill_out_read_multi_operand(cont_action, ActionAnalysis::ActionParam::PHV,
                                        read_container.toString(), mo);
            *src2_p = mo;
            src2_writebits = read_alignment.write_bits();
            if (cont_action.convert_instr_to_byte_rotate_merge)
                brm_info.src2_shift = read_alignment.right_shift / 8;
            LOG5("Src2: " << *src2_p);
        }
    }
}

/** The purpose of this function is to morph together instructions over an individual container.
 *  If multiple field actions are contained within an individual container action, then they
 *  have to be merged into an individual ALU instruction.
 *
 *  This will have to happen to instructions like the following:
 *    - set hdr.f1, param1
 *    - set hdr.f2, param2
 *
 *  where hdr.f1 and hdr.f2 are contained within the same container.  This would be translated
 *  to something along the lines of:
 *    - set $(container), $(action_data_name)
 *
 *  where $(container) is the container in which f1 and f2 are in, and $(action_data_name)
 *  is the assembly name of this combined action data parameter.  This also works between
 *  instructions with PHVs, and can even slice PHVs
 *
 *  This will also directly convert into bitmasked-sets and deposit-field.  Bitmasked-set is
 *  necessary, when the fields in the container are not contingous.  Deposit-field is necessary
 *  when there are two sources.  In any other set case, the actual instruction encoding can be
 *  directly translated from a set instruction.
 *
 *  The instruction formats are setup as a destination and two sources.  ActionAnalysis can
 *  now determine which parameters go to which source.
 */
IR::MAU::Instruction *MergeInstructions::build_merge_instruction(PHV::Container container,
         ActionAnalysis::ContainerAction &cont_action) {
    Log::TempIndent indent;
    LOG3("Building merge instruction for container : " << std::dec
            << container << " on container action : " << cont_action << indent);
    if (cont_action.is_shift()) {
        unsigned error_mask = ActionAnalysis::ContainerAction::PARTIAL_OVERWRITE;
        BUG_CHECK((cont_action.error_code & error_mask) != 0,
            "Invalid call to build a merged instruction");
        return dest_slice_to_container(container, cont_action);
    }

    const IR::Expression *dst = nullptr, *src1 = nullptr, *src2 = nullptr;
    bitvec src1_writebits, src2_writebits;
    IR::Vector<IR::Expression> components;
    ByteRotateMergeInfo brm_info;

    BUG_CHECK(cont_action.counts[ActionAnalysis::ActionParam::ACTIONDATA] <= 1, "At most "
              "one section of action data is allowed in a merge instruction");

    BUG_CHECK(!(cont_action.counts[ActionAnalysis::ActionParam::ACTIONDATA] == 1 &&
                cont_action.counts[ActionAnalysis::ActionParam::CONSTANT] >= 1), "Before "
              "merge instructions, some constant was not converted to action data");

    // Go through all PHV sources and create src1/src2 if a source is contained within these
    // PHV fields
    build_phv_source(cont_action, &src1, &src2, src1_writebits, src2_writebits, brm_info,
                     container);

    auto build_non_phv_source = [&](const IR::Expression **src, bitvec &src_writebits) {
        if (cont_action.counts[ActionAnalysis::ActionParam::ACTIONDATA] == 1) {
            build_actiondata_source(cont_action, src, src_writebits, brm_info, container);
            LOG5("ACTION DATA SOURCE for " << cont_action.name << " : " << src1_writebits);
        } else if (cont_action.counts[ActionAnalysis::ActionParam::CONSTANT] > 0) {
            // Constant merged into a single constant over the entire container
            unsigned constant_value = cont_action.ci.valid_instruction_constant(container.size());
            int width_bits;
            if ((cont_action.error_code & ActionAnalysis::ContainerAction::REFORMAT_CONSTANT) == 0
              && (cont_action.error_code & ActionAnalysis::ContainerAction::PARTIAL_OVERWRITE) == 0)
                width_bits = cont_action.ci.alignment.bitrange_cover_size();
            else
                width_bits = container.size();
            *src = new IR::Constant(IR::Type::Bits::get(width_bits), constant_value);
            src_writebits = cont_action.ci.alignment.write_bits();
            LOG5("CONSTANT SOURCE for " << cont_action.name
                    << " : " << src_writebits << ", value: " << *src);
        }
    };

    // For non commutative actions, src1 and src2 positions are fixed. Check is_commutative() on
    // ContainerAction for a list of these instructions. build_phv_source will set a source in that
    // case and we use the other source for action data / constant
    if (src1) {
        build_non_phv_source(&src2, src2_writebits);
    } else {
        build_non_phv_source(&src1, src1_writebits);
    }
    LOG5("PHV / ACTIONDATA / CONSTANT SOURCE for " << cont_action.name
            << " SRC1: " << src1 << "(" << src1_writebits << ")"
            << " SRC2: " << src2 << "(" << src2_writebits << ")");

    // Src1 is not sourced from parameters, but instead is equal to the destination
    if (cont_action.implicit_src1) {
        BUG_CHECK(src1 == nullptr, "Src1 found in an implicit_src1 calculation");
        src1 = new IR::MAU::MultiOperand(components, container.toString(), true);
        bitvec reverse = bitvec(0, container.size()) - src2_writebits;
        src1 = MakeSlice(src1, reverse.min().index(), reverse.max().index());
        src1_writebits = reverse;
        LOG5("IMPLICIT SOURCE for " << cont_action.name << " : " << src1_writebits);
    }

    // Src2 is not sources from parameters, but instead is equal to the destination
    if (cont_action.implicit_src2) {
        BUG_CHECK(src2 == nullptr, "Src2 found in an implicit_src2 calculation");
        src2 = new IR::MAU::MultiOperand(components, container.toString(), true);
    }

    auto *dst_mo = new IR::MAU::MultiOperand(components, container.toString(), true);
    fill_out_write_multi_operand(cont_action, dst_mo);
    dst = dst_mo;
    LOG5("Partial overwrite: " << cont_action.partial_overwrite()
        << " total overwrite: " << cont_action.total_overwrite_possible
        << " write bits: " << src1_writebits.popcount()
        << " container size: " << std::dec << container.size());
    // Deposit field is the only case which should allow for a slice to be generated
    if (cont_action.convert_instr_to_deposit_field
            || cont_action.is_deposit_field_variant) {
        dst = MakeSlice(dst, src1_writebits.min().index(), src1_writebits.max().index());
        LOG5("DESTINATION for " << cont_action.name << " : " << dst);
    }

    cstring instr_name = cont_action.name;
    if (cont_action.convert_instr_to_bitmasked_set)
        instr_name = "bitmasked-set"_cs;
    else if (cont_action.convert_instr_to_deposit_field)
        instr_name = "deposit-field"_cs;
    else if (cont_action.convert_instr_to_byte_rotate_merge)
        instr_name = "byte-rotate-merge"_cs;

    IR::MAU::Instruction *merged_instr = new IR::MAU::Instruction(instr_name);
    merged_instr->operands.push_back(dst);
    LOG5("PUSHED DESTINATION for " << cont_action.name << " : " << dst);

    if (!cont_action.no_sources()) {
        BUG_CHECK(src1 != nullptr, "No src1 in a merged instruction");
        merged_instr->operands.push_back(src1);
        LOG5("PUSHED SRC1 for " << cont_action.name << " : " << src1);
    }
    if (src2) {
        merged_instr->operands.push_back(src2);
        LOG5("PUSHED SRC2 for " << cont_action.name << " : " << src2);
    }
    // Currently bitmasked-set requires at least 2 source operands, or it crashes
    if (cont_action.convert_instr_to_bitmasked_set && !src2) {
        merged_instr->operands.push_back(dst);
        LOG5("A. PUSHED DESTINATION for " << cont_action.name << " (Bitmasked-Set) : " << dst);
    }

    // For a byte-rotate-merge, the last 3 opcodes are the src1 shift, the src2 shift,
    // and the src1 byte mask
    if (cont_action.convert_instr_to_byte_rotate_merge) {
        merged_instr->operands.push_back(
            new IR::Constant(IR::Type::Bits::get(container.size() / 16), brm_info.src1_shift));
        merged_instr->operands.push_back(
            new IR::Constant(IR::Type::Bits::get(container.size() / 16), brm_info.src2_shift));
        merged_instr->operands.push_back(
            new IR::Constant(IR::Type::Bits::get(container.size() / 8),
                             brm_info.src1_byte_mask.getrange(0, container.size() / 8)));
    }
    return merged_instr;
}

/** The purpose of this pass it to convert any field within an SaluAction to either a slice of
 *  phv_lo or phv_hi.  This is because a field could potentially be split in the PHV, and
 *  be a single SALU instruction.
 *
 *  This also verifies that the allocation on the input xbar is correct, by checking the location
 *  of these individual bytes and coordinating their location to guarantee correctness.
 *
 *  This assumes that the stateful alu is accessing its data through the search bus rather
 *  than the hash bus, as the input xbar algorithm can only put it on the search bus.  When
 *  we add hash matrix support for stateful ALUs, we will add that as well.
 *
 *  Also handle reduction-or of stateful ALU outputs -- having multiple stateful ALUs in the
 *  same stage write to the same action data slot, which implicitly ORs the values together.
 *  Simple `set` instructions should be used instead of `or` for those SALUs in the first
 *  stage (if split across stages, SALUs in later stages still need to `or`)
 */
const IR::Annotations *AdjustStatefulInstructions::preorder(IR::Annotations *annot) {
    prune();
    return annot;
}

const IR::MAU::IXBarExpression *AdjustStatefulInstructions::preorder(IR::MAU::IXBarExpression *e) {
    prune();
    return e;
}

/** Guarantees that all bits of a particular field are aligned correctly given a size of a
 *  field and beginning position on the input xbar
 */
bool AdjustStatefulInstructions::check_bit_positions(std::map<int, le_bitrange> &salu_inputs,
        le_bitrange field_bits, int starting_bit) {
    bitvec all_bits;
    for (auto entry : salu_inputs) {
        int ixbar_bit_start = entry.first - starting_bit;
        if (ixbar_bit_start + field_bits.lo != entry.second.lo)
            return false;
        all_bits.setrange(entry.second.lo, entry.second.size());
    }

    if (all_bits.popcount() != field_bits.size() || !all_bits.is_contiguous())
        return false;
    return true;
}

bool AdjustStatefulInstructions::verify_on_search_bus(const IR::MAU::StatefulAlu *salu,
        const Tofino::IXBar::Use &salu_ixbar, const PHV::Field *field, le_bitrange &bits,
        bool &is_hi) {
    std::map<int, le_bitrange> salu_inputs;
    bitvec salu_bytes;
    int group = 0;
    bool group_set = false;
    // Gather up all of the locations of the associated bytes within the input xbar
    for (auto &byte : salu_ixbar.use) {
        bool byte_used = true;
        for (auto fi : byte.field_bytes) {
            if (fi.field != field->name || fi.lo < bits.lo || fi.hi > bits.hi) {
                byte_used = false;
            }
        }
        if (!byte_used)
            continue;

        if (!group_set) {
            group = byte.loc.group;
            group_set = true;
        } else if (group != byte.loc.group) {
            error("Input %s for a stateful alu %s allocated across multiple groups, and "
                     "cannot be resolved", field->name, salu->name);
             return false;
        }
        salu_bytes.setbit(byte.loc.byte);
        for (auto fi : byte.field_bytes) {
            le_bitrange field_bits = { fi.lo, fi.hi };
            salu_inputs[byte.loc.byte * 8 + fi.cont_lo] = field_bits;
        }
    }

    int phv_width = salu->source_width();
    if (salu_bytes.popcount() > (phv_width / 8)) {
        error("The input %s to stateful alu %s is allocated to more input xbar bytes than the "
                "width of the ALU and cannot be resolved.", field->name, salu->name);
        return false;
    }

    if (!salu_bytes.is_contiguous()) {
        // FIXME -- we've lost the source info on 'field' so can't generate a decent error
        // message here -- should pass in `expr` from the caller to this function.
        error("The input %s to stateful alu %s is not allocated contiguously by byte on the "
                "input xbar and cannot be resolved.", field->name, salu->name);
       return false;
    }


    std::set<int> valid_start_positions;

    int initial_offset = 0;
    if (Device::currentDevice() == Device::TOFINO)
        initial_offset = Tofino::IXBar::TOFINO_METER_ALU_BYTE_OFFSET;


    valid_start_positions.insert(initial_offset);
    valid_start_positions.insert(initial_offset + (phv_width / 8));
    if (phv_width >= 64) {
        // HACK -- tofino2 flyovers for outputs allow for 4 byte chunks when in 64+ bit mode
        valid_start_positions.insert(initial_offset + 4);
        valid_start_positions.insert(initial_offset + 12); }

    if (valid_start_positions.count(salu_bytes.min().index()) == 0) {
        error("The input %s to stateful alu %s is not allocated in a valid region on the input "
                "xbar to be a source of an ALU operation", field->name, salu->name);
        return false;
    }

    if (!check_bit_positions(salu_inputs, bits, salu_bytes.min().index() * 8)) {
        error("The input %s to stateful alu %s is not allocated contiguously by bit on the "
                "input xbar and cannot be resolved.", field->name, salu->name);
        return false;
    }

    is_hi = salu_bytes.min().index() >= initial_offset + phv_width/8;
    bits = bits.shiftedByBits((salu_bytes.min().index() - initial_offset) * 8 - bits.lo);
    return true;
}

bool AdjustStatefulInstructions::verify_on_hash_bus(const IR::MAU::StatefulAlu *salu,
        const Tofino::IXBar::Use::MeterAluHash &mah, const IR::Expression *expr, le_bitrange &bits,
        bool &is_hi) {
    for (auto &exp : mah.computed_expressions) {
        const IR::Expression *pos_expr;
        if (auto *neg = exp.second->to<IR::Neg>())
            pos_expr = neg->expr;
        else
            pos_expr = exp.second;
        if (pos_expr->equiv(*expr)) {
            is_hi = exp.first != 0;
            bits = bits.shiftedByBits(exp.first - bits.lo);
            return true; } }

    BUG("The input %s to the stateful alu %s cannot be found on the hash input",
        expr, salu->name);
    return false;
}

/** The entire point of this pass is to convert any field name in a Stateful ALU instruction
 *  to either phv_lo or phv_hi, depending on the input xbar allocation.  If the field
 *  comes through the hash bus, then the stateful ALU cannot resolve the name without
 *  phv_lo or phv_hi anyway.
 */
const IR::Expression *AdjustStatefulInstructions::preorder(IR::Expression *expr) {
    visitAgain();
    if (!findContext<IR::MAU::SaluAction>()) {
        return expr;
    }

    le_bitrange bits;
    auto field = phv.field(expr, &bits);
    if (field == nullptr) {
        return expr;
    }

    auto tbl = findContext<IR::MAU::Table>();
    if (!tbl) return expr;

    auto salu = findContext<IR::MAU::StatefulAlu>();
    if (!salu) return expr;

    auto *salu_ixbar = dynamic_cast<const Tofino::IXBar::Use *>(tbl->resources->salu_ixbar.get());
    bool is_hi = false;
    const IR::Expression *pos_expr;
    cstring name = ""_cs;
    if (auto *neg = expr->to<IR::Neg>()) {
        pos_expr = neg->expr;
        name += "-phv";
    } else {
        pos_expr = expr;
        name += "phv";
    }
    if (!salu_ixbar) {
        prune();
        return expr;
    } else if (!salu_ixbar->meter_alu_hash.allocated) {
        if (!verify_on_search_bus(salu, *salu_ixbar, field, bits, is_hi)) {
            prune();
            return expr;
        }
    } else {
        if (!verify_on_hash_bus(salu, salu_ixbar->meter_alu_hash, pos_expr, bits, is_hi)) {
            prune();
            return expr;
        }
    }
    BUG_CHECK(is_hi == (bits.lo >= salu->source_width()), "inconsistent hi/bits result from "
              "verify_on_hash/search_bus");

    if (is_hi)
        name += "_hi";
    else
        name += "_lo";

    IR::MAU::SaluReg *salu_reg
        = new IR::MAU::SaluReg(expr->srcInfo, IR::Type::Bits::get(salu->source_width()), name,
                               is_hi);
    int phv_width = ((bits.size() + 7) / 8) * 8;
    salu_reg->phv_src = pos_expr;
    const IR::Expression *rv = salu_reg;
    // Sets the byte_mask for the input for the stateful alu
    if (phv_width < salu->source_width()) {
        unsigned lo = bits.lo % salu->source_width();
        rv = MakeSlice(rv, lo, lo + phv_width - 1);
    }
    prune();
    return rv;
}

class RewriteReductionOr : public MauModifier {
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    std::set<const IR::MAU::Instruction *> reduction_or;

    bool preorder(IR::MAU::Action *act) {
        reduction_or.clear();
        ActionAnalysis::ContainerActionsMap container_actions_map;
        auto tbl = findOrigCtxt<IR::MAU::Table>();
        ActionAnalysis aa(phv, true, true, tbl, red_info);
        aa.set_container_actions_map(&container_actions_map);
        act->apply(aa);

        for (auto &cact : Values(container_actions_map)) {
            if (aa.isReductionOr(cact)) {
                for (auto &fact : cact.field_actions) {
                    reduction_or.insert(fact.instruction);
                }
            }
        }
        return true;
    }
    const IR::Expression *ignoreSlice(const IR::Expression *e) {
        while (auto sl = e->to<IR::Slice>()) e = sl->e0;
        return e;
    }
    bool preorder(IR::MAU::Instruction *inst) {
        if (reduction_or.count(getOriginal<IR::MAU::Instruction>())) {
            BUG_CHECK(inst->name == "or" && inst->operands.size() == 3, "not an or: %s", inst);
            inst->name = "set"_cs;
            auto op2 = inst->operands.at(2);
            if (!ignoreSlice(op2)->to<IR::MAU::AttachedOutput>()) op2 = nullptr;
            inst->operands.resize(2);
            if (op2) inst->operands[1] = op2;
        }
        return false;
    }

 public:
    RewriteReductionOr(const PhvInfo &p, const ReductionOrInfo &ri) : phv(p), red_info(ri) {}
};

/** Eliminate Instructions */
const IR::MAU::Synth2Port* EliminateNoopInstructions::preorder(IR::MAU::Synth2Port *s) {
    LOG3("EliminateNoopInstructions preorder on synth2port: " << s);
    // Skip these tables
    prune();
    return s;
}

bool EliminateNoopInstructions::get_alloc_slice(IR::MAU::Instruction *ins,
                                    OP_TYPE type, AllocContainerSlice &op_alloc) const {
    le_bitrange op_bits;
    auto op = ins->operands[type];

    auto *field = phv.field(op, &op_bits);
    if (!field) return false;

    PHV::FieldUse use(type == DST ? PHV::FieldUse::WRITE : PHV::FieldUse::READ);

    auto tbl = findContext<IR::MAU::Table>();
    field->foreach_alloc(op_bits, tbl, &use, [&](const PHV::AllocSlice& sl) {
        le_bitrange cs = sl.container_slice();
        PHV::Container cont = sl.container();
        if (op_bits.lo <= cs.lo && op_bits.hi >= cs.hi)
            op_alloc.insert(std::make_pair(cont, cs));
    });

    if (op_alloc.empty()) return false;

    for (auto s : op_alloc)
        LOG4("  OP: " << op << " type: " << toString(type)
                        << " " << s.first << ":" << s.second);

    return true;
}

const IR::MAU::Instruction*
EliminateNoopInstructions::preorder(IR::MAU::Instruction *ins) {
    LOG3("EliminateNoopInstructions preorder on instruction: " << ins);

    int numOps = ins->operands.size();
    bool opsWithSameAlloc = false;
    auto tbl = findContext<IR::MAU::Table>();
    if (!tbl) return ins;
    if (numOps >= 2) {
        AllocContainerSlice dst_alloc, src1_alloc;

        if (!get_alloc_slice(ins, DST, dst_alloc)) return ins;
        if (!get_alloc_slice(ins, SRC1, src1_alloc)) return ins;

        opsWithSameAlloc = (dst_alloc == src1_alloc);

        if (numOps == 3) {
            AllocContainerSlice src2_alloc;
            if (!get_alloc_slice(ins, SRC2, src2_alloc)) return ins;

            opsWithSameAlloc &= (src1_alloc == src2_alloc);
        }
    }

    LOG4("  NumOps: " << numOps << " opsWithSameAlloc: "
            << opsWithSameAlloc << " ins name: " << ins->name);

    bool eliminateIns = false;
    // DST = SRC1 OR SRC2
    if (numOps == 3 && opsWithSameAlloc && ins->name == "or")   eliminateIns = true;
    // DST = SRC1 AND SRC2
    if (numOps == 3 && opsWithSameAlloc && ins->name == "and")  eliminateIns = true;
    // DST = SRC1
    if (numOps == 2 && opsWithSameAlloc && ins->name == "set")  eliminateIns = true;

    if (eliminateIns) {
        LOG3("  Instruction eliminated");
        return nullptr;
    }

    return ins;
}

/** Instruction Adjustment */
InstructionAdjustment::InstructionAdjustment(const PhvInfo &phv, const ReductionOrInfo &ri) {
    addPasses({
        new EliminateNoopInstructions(phv),
        new AdjustShiftInstructions(phv),
        new RewriteReductionOr(phv, ri),
        new SplitInstructions(phv),
        new ConstantsToActionData(phv, ri),
        new ExpressionsToHash(phv, ri),
        new MergeInstructions(phv, ri),
        new AdjustStatefulInstructions(phv)
    });
}
