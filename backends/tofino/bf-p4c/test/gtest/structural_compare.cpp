/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ir/structural_compare.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

namespace P4::Test {

TEST(TofinoStructuralCompare, TablesAndLayouts) {
    IR::MAU::Table a("a"_cs, INGRESS), b("b"_cs, INGRESS);
    a.random_seed = 10;
    b.random_seed = 0;
    EXPECT_EQ(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);

    IR::MAU::Table::Layout first, second;
    second.match_bytes = 1;  // Omitted by the original comparison.
    EXPECT_EQ(IR::structuralCompare(first, second), std::weak_ordering::less);
    EXPECT_NE(IR::structuralCompare(second, first), std::weak_ordering::less);
    first.entries = 1;  // Earlier fields take precedence.
    EXPECT_NE(IR::structuralCompare(first, second), std::weak_ordering::less);
    EXPECT_EQ(IR::structuralCompare(second, first), std::weak_ordering::less);
}

TEST(TofinoStructuralCompare, ThreadsAndTrackingIds) {
    IR::MAU::Table a("a"_cs, INGRESS), b("b"_cs, INGRESS);
    IR::MAU::TableSeq first(&a), copy(&a), second(&b);
    ASSERT_NE(first.seq_uid, copy.seq_uid);
    EXPECT_NE(first.structuralCompare(copy), std::weak_ordering::less);
    EXPECT_NE(copy.structuralCompare(first), std::weak_ordering::less);
    EXPECT_EQ(first.structuralCompare(second), std::weak_ordering::less);
    EXPECT_NE(second.structuralCompare(first), std::weak_ordering::less);

    IR::BFN::Pipe::ghost_thread_t ghost{}, equalGhost{};
    ghost.ghost_mau = &first;
    equalGhost.ghost_mau = &copy;
    EXPECT_NE(ghost.structuralCompare(equalGhost), std::weak_ordering::less);
    EXPECT_NE(equalGhost.structuralCompare(ghost), std::weak_ordering::less);

    IR::BFN::Pipe::thread_t thread{}, equalThread{};
    thread.mau = &first;
    equalThread.mau = &copy;
    EXPECT_NE(thread.structuralCompare(equalThread), std::weak_ordering::less);
    EXPECT_NE(equalThread.structuralCompare(thread), std::weak_ordering::less);
}

TEST(TofinoStructuralCompare, StaticHashIds) {
    auto *value = new IR::Constant(IR::Type_Bits::get(8), 1);
    IR::MAU::HashGenExpression a(value), b(value);
    ASSERT_NE(a.id, b.id);
    ASSERT_TRUE(a.equiv(b));
    EXPECT_NE(a.structuralCompare(b), std::weak_ordering::less);
    EXPECT_NE(b.structuralCompare(a), std::weak_ordering::less);
}

}  // namespace P4::Test
