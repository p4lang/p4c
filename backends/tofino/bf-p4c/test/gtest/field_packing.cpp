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

#include "gtest/gtest.h"

#include "ir/ir.h"
#include "bf-p4c/parde/field_packing.h"

namespace BFN {

TEST(TofinoFieldPacking, Fields) {
    FieldPacking packing;
    EXPECT_FALSE(packing.containsFields());
    EXPECT_TRUE(packing.fields.empty());
    EXPECT_EQ(0u, packing.totalWidth);

    // Any expression can serve as a field.
    auto* field1 = new IR::Constant(0);

    // Test adding one field.
    packing.appendField(field1, 10);
    EXPECT_TRUE(packing.containsFields());
    EXPECT_EQ(1u, packing.fields.size());
    EXPECT_EQ(10u, packing.totalWidth);
    EXPECT_EQ(field1, packing.fields.back().field);
    EXPECT_EQ(cstring(), packing.fields.back().source);
    EXPECT_EQ(10u, packing.fields.back().width);
    EXPECT_FALSE(packing.fields.back().isPadding());

    // Test adding a second field.
    auto* field2 = new IR::Constant(0);
    packing.appendField(field2, "field2"_cs, 1);
    EXPECT_TRUE(packing.containsFields());
    EXPECT_EQ(2u, packing.fields.size());
    EXPECT_EQ(11u, packing.totalWidth);
    EXPECT_EQ(field2, packing.fields.back().field);
    EXPECT_EQ(cstring("field2"), packing.fields.back().source);
    EXPECT_EQ(1u, packing.fields.back().width);
    EXPECT_FALSE(packing.fields.back().isPadding());

    // Test clearing all fields.
    packing.clear();
    EXPECT_FALSE(packing.containsFields());
    EXPECT_TRUE(packing.fields.empty());
    EXPECT_EQ(0u, packing.totalWidth);
}

TEST(TofinoFieldPacking, Padding) {
    FieldPacking packing;
    EXPECT_FALSE(packing.containsFields());
    EXPECT_TRUE(packing.fields.empty());
    EXPECT_EQ(0u, packing.totalWidth);

    // Test adding a chunk of padding.
    packing.appendPadding(10);
    EXPECT_FALSE(packing.containsFields());
    EXPECT_EQ(1u, packing.fields.size());
    EXPECT_EQ(10u, packing.totalWidth);
    EXPECT_TRUE(packing.fields.back().field == nullptr);
    EXPECT_EQ(cstring(), packing.fields.back().source);
    EXPECT_EQ(10u, packing.fields.back().width);
    EXPECT_TRUE(packing.fields.back().isPadding());

    // Two contiguous chunks of padding should be merged.
    packing.appendPadding(1);
    EXPECT_FALSE(packing.containsFields());
    EXPECT_EQ(1u, packing.fields.size());
    EXPECT_EQ(11u, packing.totalWidth);
    EXPECT_TRUE(packing.fields.back().field == nullptr);
    EXPECT_EQ(cstring(), packing.fields.back().source);
    EXPECT_EQ(11u, packing.fields.back().width);
    EXPECT_TRUE(packing.fields.back().isPadding());

    // A field should not be merged with padding.
    auto* field = new IR::Constant(0);
    packing.appendField(field, 5);
    EXPECT_TRUE(packing.containsFields());
    EXPECT_EQ(2u, packing.fields.size());
    EXPECT_EQ(16u, packing.totalWidth);
    EXPECT_EQ(field, packing.fields.back().field);
    EXPECT_EQ(cstring(), packing.fields.back().source);
    EXPECT_EQ(5u, packing.fields.back().width);
    EXPECT_FALSE(packing.fields.back().isPadding());

    // Two non-contiguous chunks of padding should be not merged.
    packing.appendPadding(8);
    EXPECT_TRUE(packing.containsFields());
    EXPECT_EQ(3u, packing.fields.size());
    EXPECT_EQ(24u, packing.totalWidth);
    EXPECT_TRUE(packing.fields.back().field == nullptr);
    EXPECT_EQ(cstring(), packing.fields.back().source);
    EXPECT_EQ(8u, packing.fields.back().width);
    EXPECT_TRUE(packing.fields.back().isPadding());

    // Test clearing all fields and padding.
    packing.clear();
    EXPECT_FALSE(packing.containsFields());
    EXPECT_TRUE(packing.fields.empty());
    EXPECT_EQ(0u, packing.totalWidth);
}

TEST(TofinoFieldPacking, ZeroPadding) {
    // Adding zero padding should have no effect.
    FieldPacking packing;
    packing.appendPadding(0);
    EXPECT_FALSE(packing.containsFields());
    EXPECT_TRUE(packing.fields.empty());
    EXPECT_EQ(0u, packing.totalWidth);
}

TEST(TofinoFieldPacking, EmptyPackingAlignment) {
    // An empty FieldPacking should be aligned to any number of bits. (Ignoring
    // phase, at least.) padToAlignment() should have no effect.
    for (unsigned alignment : { 1, 8, 16, 32 }) {
        SCOPED_TRACE(alignment);
        FieldPacking packing;
        EXPECT_TRUE(packing.isAlignedTo(alignment));
        packing.padToAlignment(alignment);
        EXPECT_EQ(0u, packing.totalWidth);
    }

    // An empty FieldPacking *isn't* aligned if the phase is non-zero, so
    // padToAlignment() should introduce padding bits.
    for (unsigned phase : { 0, 1, 2, 3, 4, 5, 6, 7 }) {
        SCOPED_TRACE(phase);
        FieldPacking packing;

        if (phase == 0)
            EXPECT_TRUE(packing.isAlignedTo(8, phase));
        else
            EXPECT_FALSE(packing.isAlignedTo(8, phase));

        packing.padToAlignment(8, phase);
        EXPECT_EQ(phase, packing.totalWidth);
    }
}

static void checkAlignment(const FieldPacking& packing, unsigned preAlignmentWidth,
                           unsigned alignment, unsigned phase) {
    // Check that the alignment is correct.
    EXPECT_EQ(phase, packing.totalWidth % alignment);

    // Check that we didn't add an unreasonable amount of padding. (Note
    // that this expression would need to change if the field's width
    // were greater than `alignment`.)
    EXPECT_LE(packing.totalWidth, alignment + phase);

    // If the phase is greater than the field width, we should've been
    // able to satisfy it without wrapping around to the next aligned
    // "chunk".
    if (phase > preAlignmentWidth) {
        EXPECT_LE(packing.totalWidth, alignment);
    }
}

TEST(TofinoFieldPacking, FieldAlignment) {
    // Check that we can correctly align a FieldPacking with a field in it.
    auto* field = new IR::Constant(0);
    for (unsigned alignment : { 8, 16, 32 }) {
        SCOPED_TRACE(alignment);
        for (unsigned phase : { 0, 1, 2, 3, 4, 5, 6, 7 }) {
            SCOPED_TRACE(phase);
            FieldPacking packing;
            const unsigned fieldWidth = 3;
            packing.appendField(field, fieldWidth);

            if (packing.isAlignedTo(alignment, phase)) {
                packing.padToAlignment(alignment, phase);
                EXPECT_EQ(fieldWidth, packing.totalWidth);
                EXPECT_EQ(1u, packing.fields.size());
                continue;
            }

            packing.padToAlignment(alignment, phase);
            EXPECT_TRUE(packing.containsFields());
            EXPECT_EQ(2u, packing.fields.size());
            checkAlignment(packing, fieldWidth, alignment, phase);
        }
    }
}

TEST(TofinoFieldPacking, PaddingAlignment) {
    // Check that we can correctly align a FieldPacking with padding in it.
    for (unsigned alignment : { 8, 16, 32 }) {
        SCOPED_TRACE(alignment);
        for (unsigned phase : { 0, 1, 2, 3, 4, 5, 6, 7 }) {
            SCOPED_TRACE(phase);
            FieldPacking packing;
            const unsigned paddingWidth = 3;
            packing.appendPadding(paddingWidth);

            if (packing.isAlignedTo(alignment, phase)) {
                packing.padToAlignment(alignment, phase);
                EXPECT_FALSE(packing.containsFields());
                EXPECT_EQ(paddingWidth, packing.totalWidth);
                EXPECT_EQ(1u, packing.fields.size());
                continue;
            }

            packing.padToAlignment(alignment, phase);
            EXPECT_FALSE(packing.containsFields());
            EXPECT_EQ(1u, packing.fields.size());
            checkAlignment(packing, paddingWidth, alignment, phase);
        }
    }
}

TEST(TofinoFieldPacking, ZeroAlignment) {
    // It's not clearly defined what aligning to zero bytes means, so we should
    // reject it.
    FieldPacking packing;
    packing.appendField(new IR::Constant(0), 3);
    ASSERT_ANY_THROW(packing.padToAlignment(0));
    ASSERT_ANY_THROW(packing.isAlignedTo(0));
}

TEST(TofinoFieldPacking, LargePhase) {
    // There are multiple reasonable approaches to handling a phase larger than
    // the alignment, but the simplest conceptually is just to take the phase
    // modulo the alignment, so that's what we'll do.
    FieldPacking packing;
    packing.appendField(new IR::Constant(0), 3);
    ASSERT_NO_THROW(packing.padToAlignment(8, 15));
    EXPECT_EQ(7u, packing.totalWidth);
}

TEST(TofinoFieldPacking, ForEachField) {
    // Define a packing.
    FieldPacking packing;
    packing.appendPadding(3);
    auto* field1 = new IR::Member(new IR::Constant(0), "field1"_cs);
    packing.appendField(field1, "field1"_cs, 6);
    auto* field2 = new IR::Member(new IR::Constant(0), "field2"_cs);
    packing.appendField(field2, "field2"_cs, 15);
    packing.appendPadding(9);
    auto* field3 = new IR::Member(new IR::Constant(0), "field3"_cs);
    packing.appendField(field3, "field3"_cs, 8);
    packing.padToAlignment(8);

    // Check that we can iterate over it correctly with network order ranges.
    {
        const std::vector<std::pair<cstring, nw_bitrange>> expected = {
            { "field1"_cs, nw_bitrange(StartLen(3, 6)) },
            { "field2"_cs, nw_bitrange(StartLen(9, 15)) },
            { "field3"_cs, nw_bitrange(StartLen(33, 8)) },
        };

        unsigned currentField = 0;
        packing.forEachField<Endian::Network>([&](nw_bitrange range,
                                                  const IR::Expression* field,
                                                  cstring source) {
            SCOPED_TRACE(currentField);
            ASSERT_LE(currentField, 2U);
            EXPECT_EQ(expected[currentField].first, source);
            EXPECT_EQ(expected[currentField].second, range);

            auto* member = field->to<IR::Member>();
            ASSERT_NE(member, nullptr);
            EXPECT_EQ(expected[currentField].first, member->member.name);

            currentField++;
        });
    }

    // Check that we can iterate over it correctly with little endian ranges.
    {
        const std::vector<std::pair<cstring, le_bitrange>> expected = {
            { "field1"_cs, le_bitrange(StartLen(39, 6)) },
            { "field2"_cs, le_bitrange(StartLen(24, 15)) },
            { "field3"_cs, le_bitrange(StartLen(7, 8)) },
        };

        unsigned currentField = 0;
        packing.forEachField<Endian::Little>([&](le_bitrange range,
                                                 const IR::Expression* field,
                                                 cstring source) {
            SCOPED_TRACE(currentField);
            ASSERT_LE(currentField, 2U);
            EXPECT_EQ(expected[currentField].first, source);
            EXPECT_EQ(expected[currentField].second, range);

            auto* member = field->to<IR::Member>();
            ASSERT_TRUE(member != nullptr);
            EXPECT_EQ(expected[currentField].first, member->member.name);

            currentField++;
        });
    }
}

TEST(TofinoFieldPacking, CreateExtractionState) {
    // Define a packing.
    FieldPacking packing;
    packing.appendPadding(3);
    auto* field1 = new IR::Member(new IR::Constant(0), "field1"_cs);
    packing.appendField(field1, 6);
    auto* field2 = new IR::Member(new IR::Constant(0), "field2"_cs);
    packing.appendField(field2, 15);
    packing.appendPadding(9);
    auto* field3 = new IR::Member(new IR::Constant(0), "field3"_cs);
    packing.appendField(field3, 8);
    packing.padToAlignment(8);

    // Create a parser state to extract fields according to that packing.
    auto gress = INGRESS;
    auto* finalState = new IR::BFN::ParserState("final"_cs, gress, { }, { }, { });
    cstring extractionStateName = "extract"_cs;
    auto* extractionState =
      packing.createExtractionState(gress, extractionStateName, finalState);

    // Verify that all of the state metadata (its name, the next state, the
    // amount it shifts, etc.) is correct.
    ASSERT_TRUE(extractionState != nullptr);
    EXPECT_EQ(extractionStateName, extractionState->name);
    EXPECT_EQ(gress, extractionState->gress);
    EXPECT_EQ(0u, extractionState->selects.size());
    ASSERT_EQ(1u, extractionState->transitions.size());
    EXPECT_EQ(finalState, extractionState->transitions[0]->next);
    EXPECT_TRUE(extractionState->transitions[0]->shift);
    EXPECT_EQ(static_cast<unsigned int>((packing.totalWidth / 8)),
              extractionState->transitions[0]->shift);

    // Verify that the state reproduces the packing and has the structure we
    // expect. Note that padding isn't represented as a separate IR object; it's
    // implicit in the range of bits read by an Extract.
    auto& extracts = extractionState->statements;
    ASSERT_EQ(3u, extracts.size());

    auto* field1Extract = extracts[0]->to<IR::BFN::Extract>();
    ASSERT_TRUE(field1Extract != nullptr);
    auto* field1Source = field1Extract->source->to<IR::BFN::PacketRVal>();
    ASSERT_TRUE(field1Source != nullptr);
    ASSERT_TRUE(field1Source->range == nw_bitrange(3, 8));

    auto* field2Extract = extracts[1]->to<IR::BFN::Extract>();
    ASSERT_TRUE(field2Extract != nullptr);
    auto* field2Source = field2Extract->source->to<IR::BFN::PacketRVal>();
    ASSERT_TRUE(field2Source != nullptr);
    ASSERT_TRUE(field2Source->range == nw_bitrange(9, 23));

    auto* field3Extract = extracts[2]->to<IR::BFN::Extract>();
    ASSERT_TRUE(field3Extract != nullptr);
    auto* field3Source = field3Extract->source->to<IR::BFN::PacketRVal>();
    ASSERT_TRUE(field3Source != nullptr);
    ASSERT_TRUE(field3Source->range == nw_bitrange(33, 40));
}

}  // namespace BFN
