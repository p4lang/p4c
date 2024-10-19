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

#include "bf-p4c/phv/solver/symbolic_bitvec.h"

#include "gtest/gtest.h"

namespace P4::Test {

using namespace solver::symbolic_bitvec;

TEST(symbolic_bitvec_tests, basic) {
    /*
     *    or
     *   /  \
     * {3}   0
     *
     */
    auto expr2 =
        new Expr(new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 3),
                 new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 0), ExprNodeType::OR_NODE);

    /*
     * {3}
     *
     */
    auto expr2_simpl = new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 3);

    EXPECT_TRUE(expr2->eq(expr2_simpl));

    /*
     *     or
     *   /   \
     * {2}    or
     *      /    \
     *    and     and
     *   /  \     / \
     *  1  {3}  not  0
     *           |
     *          {2}
     */
    auto *expr = new Expr(
        new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 2),
        new Expr(
            new Expr(new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 1),
                     new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 3), ExprNodeType::AND_NODE),
            new Expr(new Expr(new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 2), nullptr,
                              ExprNodeType::NEG_NODE),
                     new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 0), ExprNodeType::AND_NODE),
            ExprNodeType::OR_NODE),
        ExprNodeType::OR_NODE);

    /*
     *    or
     *   /  \
     * {2}  {3}
     *
     */
    auto expr_simpl =
        new Expr(new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 2),
                 new Expr(nullptr, nullptr, ExprNodeType::BIT_NODE, 3), ExprNodeType::OR_NODE);

    EXPECT_TRUE(expr->eq(expr_simpl));

    auto ctx = BvContext();
    const int sz = 8;
    bitvec bv;
    bv.setrange(0, 4);
    auto four_zeros = ctx.new_bv_const(4, bitvec());
    auto four_ones = ctx.new_bv_const(4, bv);
    auto mask = ctx.new_bv_const(8, bv);
    auto a = ctx.new_bv(sz);
    auto b = ctx.new_bv(sz);

    // and
    auto a1 = a.slice(0, 4);
    EXPECT_TRUE((a & mask).slice(0, 4) == a1);
    EXPECT_TRUE((a & mask).slice(4, 4) == four_zeros);
    EXPECT_TRUE((a & (~mask)).slice(0, 4) == four_zeros);
    auto a2 = a.slice(4, 4);
    EXPECT_TRUE((a & (~mask)).slice(4, 4) == a2);

    // or
    EXPECT_TRUE((a | mask).slice(0, 4) == four_ones);
    EXPECT_TRUE((a | mask).slice(4, 4) == a2);
    EXPECT_TRUE((a | (~mask)).slice(0, 4) == a1);
    EXPECT_TRUE((a | (~mask)).slice(4, 4) == four_ones);

    // left rotate
    auto a_left_rotated = a << 3;
    for (int i = 0; i < 8; i++) {
        EXPECT_TRUE(a.get(i)->eq(a_left_rotated.get((i + 3) % sz)));
    }

    // right rorate
    auto a_right_rotated = a >> 3;
    for (int i = 0; i < 8; i++) {
        EXPECT_TRUE(a.get(i)->eq(a_right_rotated.get((i + sz - 3) % sz)));
    }

    // eq
    // ~a[0] && b[0] == ~a[0] && b[0]
    EXPECT_TRUE(
        Expr(new Expr(a.get(0), nullptr, ExprNodeType::NEG_NODE), b.get(0), ExprNodeType::AND_NODE)
            .eq(new Expr(new Expr(a.get(0), nullptr, ExprNodeType::NEG_NODE), b.get(0),
                         ExprNodeType::AND_NODE)));
    // a[0] && b[0] == b[0] && a[0]
    EXPECT_TRUE(Expr(a.get(0), b.get(0), ExprNodeType::AND_NODE)
                    .eq(new Expr(b.get(0), a.get(0), ExprNodeType::AND_NODE)));
    // a[0] && b[0] != a[0] || b[0]
    EXPECT_FALSE(Expr(a.get(0), b.get(0), ExprNodeType::AND_NODE)
                     .eq(new Expr(a.get(0), b.get(0), ExprNodeType::OR_NODE)));
    // a[0] && b[0] != b[0] || a[0]
    EXPECT_FALSE(Expr(a.get(0), b.get(0), ExprNodeType::AND_NODE)
                     .eq(new Expr(b.get(0), a.get(0), ExprNodeType::OR_NODE)));
    // ~a[0] && b[0] == b[0] && ~a[0]
    EXPECT_TRUE(
        Expr(new Expr(a.get(0), nullptr, ExprNodeType::NEG_NODE), b.get(0), ExprNodeType::AND_NODE)
            .eq(new Expr(b.get(0), new Expr(a.get(0), nullptr, ExprNodeType::NEG_NODE),
                         ExprNodeType::AND_NODE)));
    // (a[0] && a[1]) && b[0] == b[0] && (a[0] && a[1])
    EXPECT_TRUE(
        Expr(new Expr(a.get(0), a.get(1), ExprNodeType::AND_NODE), b.get(0), ExprNodeType::AND_NODE)
            .eq(new Expr(b.get(0), new Expr(a.get(0), a.get(1), ExprNodeType::AND_NODE),
                         ExprNodeType::AND_NODE)));

    // example of the limitation on the equality for this library. Logically it's true,
    // but for this library, we treat them as false.
    // (a[0] && a[1]) && b[0] ?= a[0] && (b[0] && a[1])
    EXPECT_FALSE(
        Expr(new Expr(a.get(0), a.get(1), ExprNodeType::AND_NODE), b.get(0), ExprNodeType::AND_NODE)
            .eq(new Expr(a.get(0), new Expr(b.get(0), a.get(1), ExprNodeType::AND_NODE),
                         ExprNodeType::AND_NODE)));
}

}  // namespace P4::Test
