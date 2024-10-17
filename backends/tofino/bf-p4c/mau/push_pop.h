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

#ifndef BF_P4C_MAU_PUSH_POP_H_
#define BF_P4C_MAU_PUSH_POP_H_

#include "bf-p4c/common/header_stack.h"
#include "mau_visitor.h"
#include "bf-p4c/common/slice.h"

using namespace P4;

/**
 * Lowers `push_front` and `pop_front` primitives to a sequence of
 * `modify_field` primitives that perform the work.
 *
 * To understand how these primitives are implemented, it's important to
 * understand the data structure that's used for header stack POV bits. A header
 * stack's POV bits are organized in a sequential layout within a single
 * container, as follows (in little-endian order):
 *
 *  [ push bits ] [ entry 0 POV bit, entry 1 POV bit, ...] [ pop bits ]
 *
 * There is one POV bit per header stack entry. These bits are set to indicate
 * that a specific entry is valid. When the program writes to a specific header
 * stack index, we update the corresponding bit.  The bits are also used in the
 * deparser to decide which entries should be written into the output packet.
 * In other words, when you access a specific header stack entry, the POV bits
 * behave just as if they were the POV bits for a group of unrelated headers.
 *
 * When the program executes a `push_front` or `pop_front` primitive, we need to
 * shift all the entries over to make room for new values. These primitives
 * don't actually *set* any new values, though; they just make room. They could
 * just as well be called `shift_forward` or `shift_back`.
 *
 * The contents of the entries are shifted by copying each field from its old
 * position to its new position with `modify_field`. The POV bits also need to be
 * shifted, and that's what the "push bits" and "pop bits" are for. The push
 * bits are all set to 1 (that happens in the parser; see the StackPushShims
 * pass) and the pop bits are all set to 0.
 *
 * When you push, `modify_field` copies *from* the header stack POV container
 * *to* the same container. Consider the what happens when we execute
 * `push_front(header_stack, 2)`:
 *
 *                        [  SOURCE ]
 *  [ 1 1 .. push bits .. 1 1 ] [ 1 0 0 0 ] [ 0 0 .. pop bits .. 0 0 ]
 *                                [DEST ]
 *
 * The four bits labeled SOURCE - two 1's from the push bits, the 1 from entry
 * 0's POV bit, and the 0 from entry 1's POV bit - are copied over the four
 * entry POV bits, labeled DEST. Before the copy, only entry 0's POV bit was
 * set; after the copy, the POV bits for entry 0, 1, and 2 are set - we've
 * pushed two new, valid (because we'll immediate write to them) entries into
 * the header stack. In other words, the entry POV bits now look like this:
 *
 *  [ 1 1 1 0 ]
 *
 * It should be obvious that this will work correctly regardless of the existing
 * values of the entry POV bits. We just need to ensure that we have enough push
 * bits for the largest `push_front` operation we'll execute (that's why we
 * compute the `maxpush` and `maxpop` values) and that we overwrite all of the
 * existing entry POV bits when we do the update.
 *
 * The situation is analogous for `pop_front(header_stack, 3)`:
 *
 *                                      [  SOURCE ]
 *  [ 1 1 .. push bits .. 1 1 ] [ 1 1 1 1 ] [ 0 0 0 .. pop bits .. 0 0 ]
 *                                [DEST ]
 *
 * Here we're still overwriting all of the entry POV bits (labelled DEST). Since
 * we're popping three entries, we take three 0's from the pop bits and use only
 * a single entry POV bit, which is set to 1. The resulting entry POV bits:
 *
 *   [ 1 0 0 0 ]
 *
 * That's just what we'd expect after popping three entries.
 *
 * When writing the code to implement this, the layout is very important. The
 * MakeSlice calls below accept bit indices in little endian order, but the push
 * bits need to be positioned adjacent to the POV bit for the first entry, and
 * the pop bits need to be positioned adjacent to the POV bit for the last entry.
 * That means that as things stand, the container bit indices will be ordered
 * like this (though the specific indices will depend on the size of the header
 * stack, maxpush, and maxpop):
 *
 *  [ push bits ] [ entry 0 POV bit, entry 1 POV bit] [ pop bits ]
 *    5      4            3                2            1      0
 *
 * The numbering for the header stack entry POV bits moves in the opposite
 * direction from the numbering of the bits in the container!
 *
 * In the code, we name the field containing the push bits for `header_stack`
 * `header_stack.$push`. The pop bits are in `header_stack.$pop`. The entries
 * are in `header_stack[0].$valid` and so forth. A special field
 * `header_stack.$stkvalid` is overlaid over the *entire sequence of bits* so
 * that they can be referred to as a unit, and in practice we often manipulate
 * the other fields by writing to that one.
 */
class HeaderPushPop : public MauTransform {
    const BFN::HeaderStackInfo* stacks = nullptr;

    IR::BFN::Pipe* preorder(IR::BFN::Pipe* pipe) override {
        BUG_CHECK(pipe->headerStackInfo != nullptr,
                  "Running HeaderPushPop without running "
                  "CollectHeaderStackInfo first?");
        stacks = pipe->headerStackInfo;
        return pipe;
    }

    void copy_hdr(IR::Vector<IR::MAU::Primitive> *rv, const IR::Type_StructLike *hdr,
                  const IR::HeaderRef *to, const IR::HeaderRef *from) {
        for (auto field : hdr->fields) {
            auto dst = new IR::Member(field->type, to, field->name);
            auto src = new IR::Member(field->type, from, field->name);
            rv->push_back(new IR::MAU::Primitive("modify_field"_cs, dst, src)); } }
    IR::Node *do_push(const IR::HeaderRef *stack, int count) {
        auto &info = stacks->at(stack->toString());
        auto *rv = new IR::Vector<IR::MAU::Primitive>;
        for (int i = info.size-1; i >= count; --i)
            copy_hdr(rv, stack->baseRef()->type,
                     new IR::HeaderStackItemRef(stack, new IR::Constant(i)),
                     new IR::HeaderStackItemRef(stack, new IR::Constant(i - count)));
        auto *valid = new IR::Member(IR::Type::Bits::get(info.size + info.maxpop + info.maxpush),
                                     stack, "$stkvalid");
        rv->push_back(new IR::MAU::Primitive("modify_field"_cs,
            MakeSlice(valid, info.maxpop, info.maxpop + info.size - 1),
            MakeSlice(valid, info.maxpop + count, info.maxpop + info.size + count - 1)));
        return rv; }
    IR::Node *do_pop(const IR::HeaderRef *stack, int count) {
        auto &info = stacks->at(stack->toString());
        auto *rv = new IR::Vector<IR::MAU::Primitive>;
        for (int i = count; i < info.size; ++i)
            copy_hdr(rv, stack->baseRef()->type,
                     new IR::HeaderStackItemRef(stack, new IR::Constant(i - count)),
                     new IR::HeaderStackItemRef(stack, new IR::Constant(i)));
        auto *valid = new IR::Member(IR::Type::Bits::get(info.size + info.maxpop + info.maxpush),
                                     stack, "$stkvalid");
        rv->push_back(new IR::MAU::Primitive("modify_field"_cs,
            MakeSlice(valid, info.maxpop, info.maxpop + info.size - 1),
            MakeSlice(valid, info.maxpop - count, info.maxpop + info.size - count - 1)));
        return rv; }

    IR::Node *preorder(IR::MAU::Primitive *prim) override {
        BUG_CHECK(stacks != nullptr, "No HeaderStackInfo; was HeaderPushPop "
                                     "applied to a non-Pipe node?");
        if (prim->name == "push_front")
            return do_push(prim->operands[0]->to<IR::HeaderRef>(),
                           prim->operands[1]->to<IR::Constant>()->asInt());
        else if (prim->name == "pop_front")
            return do_pop(prim->operands[0]->to<IR::HeaderRef>(),
                          prim->operands[1]->to<IR::Constant>()->asInt());
        return prim; }
};

#endif /* BF_P4C_MAU_PUSH_POP_H_ */
