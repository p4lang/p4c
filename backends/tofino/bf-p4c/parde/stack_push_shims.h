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

#ifndef BF_P4C_PARDE_STACK_PUSH_SHIMS_H_
#define BF_P4C_PARDE_STACK_PUSH_SHIMS_H_

#include "parde_visitor.h"
#include "bf-p4c/common/header_stack.h"

/**
 * @ingroup parde
 * @brief Adds parser states to initialize the `$stkvalid` fields that are used to
 *        handle the `push_front` and `pop_front` primitives for header stacks.
 *
 * @see HeaderPushPop for more discussion.
 */
class StackPushShims : public PardeModifier {
    const BFN::HeaderStackInfo* stacks = nullptr;

    bool preorder(IR::BFN::Pipe* pipe) override {
        BUG_CHECK(pipe->headerStackInfo != nullptr,
                  "Running StackPushShims without running "
                  "CollectHeaderStackInfo first?");
        stacks = pipe->headerStackInfo;
        return true;
    }

    bool preorder(IR::BFN::Parser *p) override {
        BUG_CHECK(stacks != nullptr, "No HeaderStackInfo; was StackPushShims "
                                     "applied to a non-Pipe node?");
        for (auto &stack : *stacks) {
            if (stack.maxpush == 0 || !stack.inThread[p->gress]) continue;

            // The layout of `$stkvalid` is an overlay over a header stack's
            // `$push` field, its entry POV bits, and its `$pop` field:
            //
            // [ 1 1 .. $push .. 1 1 ] [ .. entry POVs .. ] [ 0 0 .. $pop .. 0 0 ]
            //
            // We need to initialize all of the `$push` bits to 1, but we need to
            // do it using a write to `$stkvalid`, because in the MAU we'll
            // access these bits only through `$stkvalid` and if we write to
            // `$push` we'll end up thinking that write is dead and eliminate
            // it.
            const unsigned pushValue = (1U << stack.maxpush) - 1;
            const unsigned stkValidSize = stack.size + stack.maxpush + stack.maxpop;
            const unsigned stkValidValue = pushValue << (stack.size + stack.maxpop);

            IR::Vector<IR::BFN::ParserPrimitive> initStkValid = {
                new IR::BFN::Extract(
                    new IR::Member(IR::Type::Bits::get(stkValidSize),
                                   new IR::PathExpression(stack.name), "$stkvalid"),
                    new IR::BFN::ConstantRVal(stkValidValue))
            };
            p->start = new IR::BFN::ParserState(stack.name + "$shim", p->gress,
                                                initStkValid, { }, {
                new IR::BFN::Transition(match_t(), 0, p->start)
            });
        }
        return false; }
};

#endif /* BF_P4C_PARDE_STACK_PUSH_SHIMS_H_ */
