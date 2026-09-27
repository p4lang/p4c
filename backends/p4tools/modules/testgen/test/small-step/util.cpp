// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

namespace P4::P4Tools::Test {

TEST_F(SmallStepTest, PacketSliceSurvivesAssignmentSimplification) {
    auto context = P4TestgenTest::SetUp("bmv2", "v1model");
    ASSERT_NE(context, nullptr);
    IR::P4Program program;
    IR::MutablePtr<ExecutionState> state = &ExecutionState::create(&program);
    state->appendToPacketBuffer(IR::Constant::get(IR::Type_Bits::get(8), 42));
    auto slice = state->slicePacketBuffer(8);
    ASSERT_TRUE(slice->is<IR::Slice>());
    const IR::StateVariable field(
        new IR::Member(IR::Type_Bits::get(8), new IR::PathExpression("header"), "field"));
    state->set(field, slice);
    EXPECT_EQ(state->get(field)->checkedTo<IR::Constant>()->value, 42);
    // set() replaces the slice with a constant. The caller still owns the slice for tracing.
    EXPECT_EQ(slice->checkedTo<IR::Slice>()->getH(), 7);
    EXPECT_EQ(slice->checkedTo<IR::Slice>()->getL(), 0);
}

}  // namespace P4::P4Tools::Test
